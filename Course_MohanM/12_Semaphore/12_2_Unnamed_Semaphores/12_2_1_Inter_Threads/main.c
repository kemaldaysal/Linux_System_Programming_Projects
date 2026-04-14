/* NOTES
* Named semaphores live in the file system, unnamed semaphore is a sem_t object stored in some memory region (shared memory?)
    * So it's like a variable and it can be present in different memory segments
* Can be used:
    * Within the process
    * Used to synchronize betweeen different threads
        * Should be kept in the place where it is accessible to all threads (global memory area)
    * Synchronize between different threads of different processes.
        * Should be placed in a memory where the memory is accessible to both processes. (intro to shared memory concept)

* Uses most of the same functions with unnammed semaphores (sem_wait(), sem_post(), sem_getvalue(), ...),
    * But the creation and destroy functions are different (sem_init(), sem_destroy(sem))

* In "int sem_init(sem_t *sem, int pshared, unsigned int init_value)",
    * pshared indicates whether the semaphore is to be shared between threads (0) or between processes (!= 0)
        * For inter-process use, pshared != 0 alone is not enough. The semaphore object must also physically reside in shared memory.

* It is a good practice to destroy a semaphore (sem_destroy()) only if no processes or threads are waiting on it.
    * Worker threads do not call a function like sem_close() for unnamed semaphores.
    * There is nothing “opened by name” per-thread that must be closed.
    * In this example, since main joins all worker threads before destroying the semaphore, it knows all threads have finished using it.

* Do not interfere semaphore with mutex, they are not the same and it can lead to a race condition, read and review it in your ChatGPT notes!!
    * We used mutex exclusively since it prevents race conditions that updates global variable when parameter initial semaphore count > 1

*/

// ---- Libraries and headers ----

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>

// ---- Settings and Defines ----

#define LOOP_COUNT 1000000

#define SVAL_INITIAL 2

enum PSHARED_SETTING
{
    PSHARED_BETWEEN_THREADS = 0,
    PSHARED_BETWEEN_PROCESSES = 1
};

enum RETURN_TYPES
{
    SUCCESS = 0,
    ERROR_GENERAL = -1,
};

#define CNT_THREADS 3
#define BUF_SIZE 7

// ---- Globals ----

static int glob = 0;
sem_t sem;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

// ---- Function Prototypes ----

static void *threadFunc(void *arg);

// ----- Main Function -----
int main()
{

    printf("M(%lu): Started executing...\n", (unsigned long)pthread_self());

    printf("M(%lu): Initializing semaphore...\n", (unsigned long)pthread_self());

    if (sem_init(&sem, PSHARED_BETWEEN_THREADS, SVAL_INITIAL) == ERROR_GENERAL)
    {
        perror("Error during sem_init");
        exit(EXIT_FAILURE);
    }

    printf("M(%lu): Creating %d threads with arguments passed...\n", (unsigned long)pthread_self(), CNT_THREADS);

    pthread_t t[CNT_THREADS];
    char args[CNT_THREADS][BUF_SIZE];

    for (size_t i = 0; i < CNT_THREADS; i++)
    {
        snprintf(args[i], BUF_SIZE, "thr%zu", i + 1);
        if (pthread_create(&t[i], NULL, threadFunc, args[i]) != SUCCESS)
        {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    for (size_t i = 0; i < CNT_THREADS; i++)
    {
        if (pthread_join(t[i], NULL) != SUCCESS)
        {
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
    }

    sem_destroy(&sem);
    pthread_mutex_destroy(&mtx);
    
    printf("M(%lu): After all threads finished, glob: %d\n", (unsigned long)pthread_self(), glob);

    return EXIT_SUCCESS;
}

// ----- Helper Functions -----

// --- Thread Handler ---
static void *threadFunc(void *arg)
{
    printf("T(%lu): Started executing...\n", (unsigned long)pthread_self());
    printf("T(%lu): Passed argument is: %s\n", (unsigned long)pthread_self(), (char *)arg);
    printf("T(%lu): Waiting for semaphore resources...\n", (unsigned long)pthread_self());
    if (sem_wait(&sem) != 0)
    {
        printf("T(%lu): Semaphore wait error!!\n", (unsigned long)pthread_self());
    }

    printf("T(%lu): Semaphore availed, starting to execute critical section...\n", (unsigned long)pthread_self());

    int local = 0;
    for (int j = 0; j < LOOP_COUNT; j++)
    {
        local++;
    }

    printf("T(%lu): Waiting for mutex to update global variable...\n", (unsigned long)pthread_self());
    pthread_mutex_lock(&mtx);
    printf("T(%lu): Mutex acquired, updating global value...\n", (unsigned long)pthread_self());
    glob += local;
    pthread_mutex_unlock(&mtx);

    if (sem_post(&sem) != 0)
    {
        printf("T(%lu): Semaphore post error!!\n", (unsigned long)pthread_self());
    }

    printf("T(%lu): Critical section ended, semaphore released, exiting...\n\n", (unsigned long)pthread_self());
    return NULL;
}