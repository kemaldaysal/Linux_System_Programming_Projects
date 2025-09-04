/* ---- Notes ----
 * It's advised to read notes written on previous script
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

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_from_t1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_from_main = PTHREAD_COND_INITIALIZER;
bool t1_ready = 0;
bool t1_to_proceed = 0;
bool t1_done = 0;

/* ----- Main Function ---- */

int main()
{
    printf("-MT (pid: %d, thread id: %lu) : Started.\n", getpid(), (unsigned long)pthread_self());

    pthread_t t1;

    // int retval = pthread_create(&t1, NULL, thread_function, "Arg Test");
    int val_to_pass = 5;
    CHECK_ERR(pthread_create(&t1, NULL, thread_function, &val_to_pass), "Error creating thread");

    printf("-MT : Thread t1 is created successfully, waiting for t1's ready signal...\n");

    CHECK_ERR(pthread_mutex_lock(&mutex), "Lock failed");
    while (t1_ready != 1)
    {
        CHECK_ERR(pthread_cond_wait(&cond_from_t1, &mutex), "Error waiting");
    }

    t1_to_proceed = 1;
    CHECK_ERR(pthread_cond_signal(&cond_from_main), "Signal to t1 failed");
    CHECK_ERR(pthread_mutex_unlock(&mutex), "Unlock failed"); // NOT UNNECESSARY !! Read your notes

    printf("-MT : t1 had reported it was ready, so i sent the start working signal to him.\n");
    printf("-MT : Will be waiting for t1 to finish it's job...\n");

    CHECK_ERR(pthread_mutex_lock(&mutex), "Lock failed"); // NOT UNNECESSARY !! Read your notes

    while (t1_done != 1)
    {
        CHECK_ERR(pthread_cond_wait(&cond_from_t1, &mutex), "Error waiting");
    }

    CHECK_ERR(pthread_mutex_unlock(&mutex), "Unlock failed");
    printf("-MT : t1 reported done. \n");
    printf("-MT : Cleaning up t1's resources with join.\n");

    void *thread_result;
    CHECK_ERR(pthread_join(t1, &thread_result), "Thread join failed");

    printf("-MT : T1 exited with status %ld\n", (long)thread_result);
    printf("-MT : Exiting\n");

    pthread_exit((void *)EXIT_SUCCESS);
}

/* ----- Function Implementations ---- */

static void *thread_function(void *arg)
{
    // char *passed_val = (char*) arg;
    int passed_val = *((int *)arg);

    printf("T1 (pid: %d, thread id: %lu) : Started executing\n", getpid(), (unsigned long)pthread_self());
    printf("T1 : Ready to do work...\n");
    CHECK_ERR(pthread_mutex_lock(&mutex), "Lock failed");
    t1_ready = 1;
    CHECK_ERR(pthread_cond_signal(&cond_from_t1), "Signal to main failed");

    while (t1_to_proceed != 1)
    {
        CHECK_ERR(pthread_cond_wait(&cond_from_main, &mutex), "Wait for proceed signal from main failed");
    }

    CHECK_ERR(pthread_mutex_unlock(&mutex), "Unlock failed");

    printf("T1 : Go signal from main received, starting to do work.\n");

    printf("T1 : Passed arguments from main are: %d\n", passed_val);
    printf("T1 : Work done, signaling main\n");

    CHECK_ERR(pthread_mutex_lock(&mutex), "Lock failed");
    t1_done = 1;
    CHECK_ERR(pthread_cond_signal(&cond_from_t1), "Signal to main failed");
    CHECK_ERR(pthread_mutex_unlock(&mutex), "Unlock failed");

    printf("T1 : Exiting.\n");
    pthread_exit((void *)EXIT_SUCCESS);
}