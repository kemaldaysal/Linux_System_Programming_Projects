/* ----- Libraries -----*/

#define _POSIX_C_SOURCE 200112L // for timedlock

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <time.h>

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

/* ----- Function Prototypes -----*/

void *thread_func_t1(void *arg);
void *thread_func_t2(void *arg);

/* ----- Global (Shared) Variables -----*/

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* ----- Main Function -----*/

int main()
{
    printf("MT (pid = %d , tid = %ld) : Started executing...\n", getpid(), (unsigned long)pthread_self());
    printf("MT : Creating t1...\n");

    pthread_t t1;
    pthread_t t2;

    CHECK_ERR(pthread_create(&t1, NULL, &thread_func_t1, NULL), "Error creating t1");
    CHECK_ERR(pthread_detach(t1), "Error detaching t1");

    sleep(1);

    CHECK_ERR(pthread_create(&t2, NULL, &thread_func_t2, NULL), "Error creating t2");
    CHECK_ERR(pthread_detach(t2), "Error detaching t2");

    printf("MT : Exiting...\n");

    pthread_exit((void *)EXIT_SUCCESS);
}

/* ----- Function Implementations -----*/

void *thread_func_t1(void *arg)
{
    printf("-T1 (tid = %ld) : Started executing.\n", (unsigned long)pthread_self());
    printf("-T1 : Locking mutex...\n");

    pthread_mutex_lock(&mutex);
    while (1)
        ; // Suppose that it's stuck after locking mutex, before unlocking it.
    pthread_mutex_unlock(&mutex);

    printf("-T1 : Locked and unlocked mutex successfully.\n");
    printf("-T1 : Exiting...");

    pthread_exit((void *)EXIT_SUCCESS);
}

void *thread_func_t2(void *arg)
{
    printf("--T2 (tid = %ld) : Started executing...\n", (unsigned long)pthread_self());

    printf("--T2 : Trying to take ownership of mutex and lock it...\n");

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 2; // wait for 2 seconds max

    // pthread_mutex_lock(&mutex); // with this way, it won't take the ownership since t1 is made it dead-lock.

    // --- You can try below 2 functions and see their behaviour by uncommenting them.

    CHECK_ERR(pthread_mutex_timedlock(&mutex, &ts), "--T2 : Error locking mutex"); // Tries to lock and stays in blocked state until timeout it out, then returns error if can't lock
    // CHECK_ERR(pthread_mutex_trylock(&mutex), "--T2 : Error locking mutex"); // Tries to lock immediately and gives error if locking is failed and mutex is busy, returns error and avoids deadlock.

    CHECK_ERR(pthread_mutex_unlock(&mutex), "--T2 : Error unlocking mutex");
    printf("--T2 : Locked and unlocked mutex successfully.\n");
    printf("--T2 : Exiting...");

    pthread_exit((void *)EXIT_SUCCESS);
}
