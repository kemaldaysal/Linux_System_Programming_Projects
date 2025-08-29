/* ----- Notes -----
    -Keep in mind that cancellation isn't precise and it depends on scheduling.
    It's inherently non-deterministic,
    -When you call pthread_cancel(t1), the target thread doesn’t vanish at that exact CPU instruction.
    Instead, the cancel request is delivered, and the target thread will terminate the next time it hits a cancellation point.

    -So the reason you see counter at 44, 87, 148 … is just OS scheduler randomness.
    The thread ran a little further before hitting pthread_testcancel(). It’s not a bug, it’s how cancellation works.

    -Therefore, don’t use cancellation if you need precise control (e.g., "stop exactly at 50").
    For that, you should use your own shared flag (e.g. volatile bool stop = true;) checked in the worker loop.
    -pthread_cancel is cooperative but asynchronous: you don’t control the exact timing, so state may be left in awkward conditions.
    With a custom flag + mutex/condvars, you decide exactly when and how the thread stops. That’s predictable and deterministic.
    -That’s why production code (especially in embedded or safety-critical systems) avoids pthread_cancel() almost entirely
    Instead, they implement their own stop/exit signaling.

    -For cleaning up the resources after cancelling,
    If the thread is detached, all its resources are automatically reclaimed after cancel.
    If it’s joinable, you’d still need to pthread_join() to release them.
    So cancellation is still a normal termination path as far as the system is concerned. Detach covers cleanup.

    About cleanup handlers after cancellation
    -if you don’t own heap allocations, open file descriptors, or locks that must be released, cleanup handlers are unnecessary.
    But if your thread holds a mutex, has malloc’d memory, opened a file/socket, or needs to log a message on shutdown,
    Then a cleanup handler is the only reliable way to ensure those resources are released even if canceled halfway through.
    Otherwise you risk deadlocks or leaks (which you're already risking it since
    your current code can deadlock if pthread_cancel() happens while t1 is inside a critical section holding the mutex)

    pthread_cancel() only takes effect at a cancellation point.
    In your loop, the only cancellation point is pthread_testcancel(), after you release the mutex.
    So as written, it looks “safe” because cancellation won’t fire in the middle of a locked section.
    BUT: if later you add another cancellation point (e.g. pthread_cond_wait, read, sleep, etc.) inside the locked section,
        then cancellation could fire while holding the lock → main would block forever waiting for mutex.
    That’s why POSIX recommends pairing locks with cleanup handlers: they guarantee that resources (like mutexes) are released
        if cancellation fires at the wrong time.
    This code without cleanup handlers already avoids deadlock only because pthread_testcancel() happens outside the lock.
    With a cleanup handler, we made it robust against future changes (adding blocking calls inside the lock).
    This is the standard way to make pthread_cancel safe in code that uses mutexes.
*/

/* ----- Libraries -----*/

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/* ----- Macros -----*/

#define CHECK_ERR(err, msg)                                  \
    do                                                       \
    {                                                        \
        if (err != 0)                                        \
        {                                                    \
            fprintf(stderr, "%s: %s\n", msg, strerror(err)); \
            exit(EXIT_FAILURE);                              \
        }                                                    \
    } while (0)

#define MAX_COUNT 150
#define COUNT_TO_STOP 50

/* ----- Function Prototypes -----*/

static void *thread_function(void *arg);
static void cleanup_mutex(void *arg);

/* ----- Global (Shared) Variables -----*/

bool t1_ready = 0;
bool t1_to_continue = 0;
size_t counter = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_from_t1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_from_main = PTHREAD_COND_INITIALIZER;

/* ----- Main Function -----*/

int main()
{
    printf("MT (pid = %d, tid = %ld) : Started executing...\n", getpid(), (unsigned long)pthread_self());
    pthread_t t1;
    CHECK_ERR(pthread_create(&t1, NULL, thread_function, NULL), "Error creating the thread");
    CHECK_ERR(pthread_detach(t1), "Error setting thread as detachable"); // Set created thread as detachable

    CHECK_ERR(pthread_mutex_lock(&mutex), "Error locking mutex");
    while (t1_ready != 1)
    {
        CHECK_ERR(pthread_cond_wait(&cond_from_t1, &mutex), "Error waiting ready signal from t1");
    }
    t1_to_continue = 1;
    CHECK_ERR(pthread_cond_signal(&cond_from_main), "Error when sending signal");
    CHECK_ERR(pthread_mutex_unlock(&mutex), "Error unlocking mutex");

    printf("MT : I commanded t1 to start it's job.\n");

    printf("MT : I'll terminate t1 when the counter reaches %d.\n", COUNT_TO_STOP);
    CHECK_ERR(pthread_mutex_lock(&mutex), "Error locking mutex");
    while (counter < COUNT_TO_STOP)
    {
        // printf("MT : Shared counter: %lu\n", counter);
        CHECK_ERR(pthread_cond_wait(&cond_from_t1, &mutex), "Error waiting counter signal from t1");
    }
    CHECK_ERR(pthread_cancel(t1), "Error canceling the thread"); // Cancel thread immediately or at the next possibility.
    CHECK_ERR(pthread_mutex_unlock(&mutex), "Error unlocking mutex");
    printf("MT : I've terminated t1 when counter reached %lu. main thread exiting...\n", counter);

    pthread_exit((void *)EXIT_SUCCESS);
}

/* ----- Function Implementations -----*/

static void *thread_function(void *arg)
{
    printf("-T1 (tid = %ld) : Started executing, notifying that i'm ready.\n", (unsigned long)pthread_self());
    pthread_mutex_lock(&mutex);
    t1_ready = 1;
    pthread_cond_signal(&cond_from_t1);

    while (t1_to_continue != 1)
    {
        pthread_cond_wait(&cond_from_main, &mutex);
    }
    pthread_mutex_unlock(&mutex);

    printf("-T1 : Both of us are ready, starting counter...\n");

    for (size_t i = 0; i < MAX_COUNT; i++)
    {
        CHECK_ERR(pthread_mutex_lock(&mutex), "Error locking mutex");
        // register cleanup in case cancel occurs while mutex is held
        pthread_cleanup_push(cleanup_mutex, &mutex);

        counter++;
        // printf("-T1 : Shared counter: %lu\n", counter);
        CHECK_ERR(pthread_cond_signal(&cond_from_t1), "Error when sending signal");
        CHECK_ERR(pthread_mutex_unlock(&mutex), "Error unlocking mutex");

        // remove cleanup since we manually unlocked
        pthread_cleanup_pop(0);

        pthread_testcancel(); // to allow timely cancellation from main thread
    }

    printf("-T1 : Counting job has ended, exiting...\n");

    // pthread_detach(pthread_self()); // It can also set itself as detachable but it's more optimal to do it after creation in main
    pthread_exit((void *)EXIT_SUCCESS);
}

static void cleanup_mutex(void *arg)
{
    pthread_mutex_t *m = arg;
    pthread_mutex_unlock(m);
    printf("-T1 : cleanup handler released mutex after cancellation.\n");
}