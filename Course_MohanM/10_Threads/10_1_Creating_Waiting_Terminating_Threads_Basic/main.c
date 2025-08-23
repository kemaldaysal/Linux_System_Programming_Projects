/* ---- Notes ----

    *Thread shares the virtual memory of the process. 
    *Created thread has the same process ID with the main thread, since they share the same virtual memory but different stacks for each of the threads.
    *More commonly, threads communicate results via shared memory (protected with mutexes/condition variables) or message queues — not via the return value.
    *pthread_join blocks the main thread until t1 exits, ensuring proper synchronization.
    *For finer-grained signaling (not just exit), pthread_cond_wait, phread_cond_signal, semaphores (sem_t), condition variables, eventfd/pipe are used 
    *For basic demonstration, we passed a string literal "Arg Test" directly, but in real code, we shouldn't pass stack addresses of locals unless we guarantee lifetime. For dynamic arguments, we should do allocate and free inside thread.
    *Return type of thread_function must be void * so that the thread can return any kind of pointer-sized result back to whoever calls pthread_join.
    
    * Returning from thread function or calling pthread_exit() are equivalent.    
    
    */

/* ----- Libraries ---- */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

/* ----- Macros & Macro Functions ---- */

#define CHECK_ERR(err, msg) \
    do { if (err != 0) { fprintf(stderr, "%s: %s\n", msg, strerror(err)); exit(EXIT_FAILURE); } } while(0)

/* ----- Function Prototypes ---- */

static void *thread_function(void *arg);

/* ----- Main Function ---- */

int main()
{
    printf("Main Thread (pid: %d, thread id: %lu) : Started.\n", getpid(), (unsigned long) pthread_self());

    pthread_t t1;

    // int retval = pthread_create(&t1, NULL, thread_function, "Arg Test");
    int val_to_pass = 5;
    int retval = pthread_create(&t1, NULL, thread_function, &val_to_pass);
    CHECK_ERR(retval, "Error creating thread");

    printf("Main Thread : Thread t1 is created successfully, waiting for t1 to finish...\n");

    void *thread_result;
    retval = pthread_join(t1, &thread_result);
    CHECK_ERR(retval, "Error joining thread");

    printf("Main Thread : t1 exited with status %ld\n", (long int) thread_result);
    printf("Main Thread : Exiting.\n");

    // exit(EXIT_SUCCESS); // If main finishes earlier, it'll not wait for other threads to finish and terminate them.  
    pthread_exit((void*) EXIT_SUCCESS); // To let the other threads finish, if main finishes earlier.
}

/* ----- Function Implementations ---- */

static void *thread_function(void *arg)
{
    // char *passed_val = (char*) arg;
    int passed_val = *((int*) arg);

    printf("T1 (pid: %d, thread id: %lu) : Started executing\n", getpid(), (unsigned long) pthread_self());

    // printf("T1 : Passed arguments from main are: %s\n", passed_val);
    printf("T1 : Passed arguments from main are: %d\n", passed_val);
    printf("T1 : Exiting.\n");

    pthread_exit((void*)EXIT_SUCCESS);
}