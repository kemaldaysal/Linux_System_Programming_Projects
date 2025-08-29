/* ---- Notes ----
 * It's almost the same script as 10_3, but includes dynamic mutexes and condition variables instead of statics of them.
 * WARNING !! Don't forget to destroy mutexes and condition variables after "ALL THREADS FINISH USING THEM"!!
 * WARNING !! If main exits and destroys the synchronization primitives before t1 is done, you’ve broken the program.
 * A mutex/cond var must stay valid until all threads that might use it are finished.
 * Only after that point should you destroy it.
 * So using joinable threads are safer here rather than detachable threads. Or you should guarantee the lifetime rules.
 * If you want detached threads, then:
 *   Either never destroy the mutex/cond (let the OS reclaim resources when process ends).
 *   Or make the thread itself responsible for destroying its own mutex/cond, since only it knows when it’s done.
 *   Or wrap everything in a heap-allocated struct, and the detached thread frees it when finished.
 *      So in this example, we'll use a heap-allocated struct to demonstrate the real life usage.
 */

/* ----- Libraries ---- */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

/* ----- Macros & Macro Functions ---- */

#define CHECK_ERR(err, msg)                                  \
    do                                                       \
    {                                                        \
        if (err != 0)                                        \
        {                                                    \
            fprintf(stderr, "%s: %s\n", msg, strerror(err)); \
            exit(EXIT_FAILURE);                              \
        }                                                    \
    } while (0)

/* ----- Function Prototypes ---- */

static void *thread_function(void *arg);

/* ----- Globals (shared between threads) ----- */

typedef struct
{
    pthread_mutex_t mutex;
    pthread_cond_t cond_from_t1;
    pthread_cond_t cond_from_main;
    bool t1_ready;
    bool t1_to_proceed;
    bool t1_done;
    int arg;
} shared_data_t;

/* ----- Main Function ---- */

int main()
{
    printf("-MT (pid: %d, thread id: %lu) : Started.\n", getpid(), (unsigned long)pthread_self());

    /* Allocate shared data dynamically */
    shared_data_t * data = malloc(sizeof(shared_data_t));
    if (!data)
    {
        perror("Malloc failed");
        exit(EXIT_FAILURE);
    }


    CHECK_ERR(pthread_mutex_init(&(data->mutex), NULL), "Mutex init failed");
    CHECK_ERR(pthread_cond_init(&(data->cond_from_main), NULL), "cond init failed (main)");
    CHECK_ERR(pthread_cond_init(&(data->cond_from_t1), NULL), "cond init failed (t1)");
    data->t1_done = data->t1_ready = data->t1_to_proceed = 0;
    data->arg = 42;

    pthread_t t1;

    CHECK_ERR(pthread_create(&t1, NULL, thread_function, data), "Error creating thread");
    CHECK_ERR(pthread_detach(t1), "Error detaching thread");

    printf("-MT : Thread t1 is created successfully, waiting for t1's ready signal...\n");

    CHECK_ERR(pthread_mutex_lock(&(data->mutex)), "Lock failed");
    while (data->t1_ready != 1)
    {
        CHECK_ERR(pthread_cond_wait(&(data->cond_from_t1), &(data->mutex)), "Error waiting");
    }

    data->t1_to_proceed = 1;
    CHECK_ERR(pthread_cond_signal(&(data->cond_from_main)), "Signal to t1 failed");
    CHECK_ERR(pthread_mutex_unlock(&(data->mutex)), "Unlock failed"); // NOT UNNECESSARY !! Read your notes

    printf("-MT : t1 had reported it was ready, so i sent the start working signal to him.\n");
    printf("-MT : Exiting\n");

    pthread_exit((void *)EXIT_SUCCESS);
}

/* ----- Function Implementations ---- */

static void *thread_function(void *arg)
{
    shared_data_t * data = ((shared_data_t*) arg);

    printf("T1 (pid: %d, thread id: %lu) : Started executing\n", getpid(), (unsigned long)pthread_self());
    printf("T1 : Ready to do work...\n");
    CHECK_ERR(pthread_mutex_lock(&(data->mutex)), "Lock failed");
    data->t1_ready = 1;
    CHECK_ERR(pthread_cond_signal(&(data->cond_from_t1)), "Signal to main failed");

    while (data->t1_to_proceed != 1)
    {
        CHECK_ERR(pthread_cond_wait(&(data->cond_from_main), &(data->mutex)), "Wait for proceed signal from main failed");
    }

    CHECK_ERR(pthread_mutex_unlock(&(data->mutex)), "Unlock failed");

    printf("T1 : Go signal from main received, starting to do work.\n");

    printf("T1 : Passed argument from main is: %d\n", data->arg);
    printf("T1 : Work done.\n");
    printf("T1 : Started cleanup...\n");

    /* Thread dones the cleanup since it's a heap address that both threads can reach anytime if the pointer is passed to them!! */
    CHECK_ERR(pthread_mutex_destroy(&(data->mutex)), "Mutex destroy failed");
    CHECK_ERR(pthread_cond_destroy(&(data->cond_from_main)), "Cond (from main) destroy failed");
    CHECK_ERR(pthread_cond_destroy(&(data->cond_from_t1)), "Cond (from t1) destroy failed");
    free(data);

    printf("T1 : Exiting...\n");
    return NULL;
}