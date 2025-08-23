/* ---- Notes ----

    Thread shares the virtual memory of the process. 
    Created thread has the same process ID with the main thread, since they share the same virtual memory but different stacks for each of the threads.
    More commonly, threads communicate results via shared memory (protected with mutexes/condition variables) or message queues — not via the return value.
    pthread_join blocks the main thread until t1 exits, ensuring proper synchronization.
    For finer-grained signaling (not just exit), pthread_cond_wait, phread_cond_signal, semaphores (sem_t), condition variables, eventfd/pipe are used 
    For basic demonstration, we passed a string literal "Arg Test" directly, but in real code, we shouldn't pass stack addresses of locals unless we guarantee lifetime. For dynamic arguments, we should do allocate and free inside thread.
    Return type of thread_function must be void * so that the thread can return any kind of pointer-sized result back to whoever calls pthread_join.
    
    pthread_cond_wait / pthread_cond_signal let threads "handshake" on events.
    Returning from thread function or calling pthread_exit() are equivalent.    
     
    Advantages over previous code
    pthread_join only waits for a thread to exit completely. That’s fine if all you care about is “thread finished, clean it up”.
    Condition variables let you signal at any point during execution (not just at exit). 
    Example: A worker thread processes multiple jobs in a loop and signals the main thread after each job. 
    You cannot do that with pthread_join, since join only works once and only when the thread terminates.
    Advantage: Condition variables are more flexible for fine-grained synchronization, not just “end-of-thread”. This is why they’re widely used in embedded and real-time systems.    
    
    The condition variable (pthread_cond_t cond) itself is just a signaling mechanism.
    The shared state (work_done) is what you actually care about.
    To avoid race conditions, you must protect that shared state with a mutex.
    
    Also, if you didn’t signal, then the main thread would need to poll, or sleep and check periodically. That wastes CPU and is unreliable.
    Instead, with a condition variable: 
        The main thread sleeps efficiently inside pthread_cond_wait.
        When T1 signals, the kernel wakes up the main thread immediately.
        That way, there’s no polling, no wasted CPU cycles.
            So, work_done = 1; is the shared state (the “what”).
            pthread_cond_signal is the notification mechanism (the “when”).
            pthread_cond_signal ensures main wakes up at the right time and re-checks the variable safely under the mutex.            
    You need both:
        Without the variable, main doesn’t know what happened.
        Without the signal, main might sleep forever even though the variable changed.
    
    --Mutex on t1
    Lock mutex, change shared state to 1, signal condition (inform main thread that shared state is changed), unlock mutex.

    --Mutex on main function
    Without mutex, you can have timing bugs: 
        Main thread checks work_done before it was updated.
        Signal is missed if sent before pthread_cond_wait was actually blocking.
        Mutex ensures no race between reading/writing a shared state.
    So signal is not the “cond” itself — the condition variable (cond) is just a mechanism.
    The real condition is your shared state (work_done).
    --Why do we need pthread_cond_signal?
        Because without it, the waiting thread will never wake up.
        The signal is essentially the doorbell telling main thread:
            "The condition (work_done=1) you were waiting for is now true. Wake up and check."
    
    --Volatiles
        volatile is often misunderstood. In multithreaded code: 
        Volatile only prevents the compiler from optimizing away reads/writes. 
        It does not guarantee visibility between threads, because CPUs and caches can reorder or delay memory operations. 
        Thread T1 could update work_done = 1;, but due to caching or instruction reordering, main might never see it in time.
        Mutexes and condition variables provide memory barriers that force changes to become visible to other threads in the correct order.
    
        So in professional multithreading: 
            We use mutex + cond (or atomics) to enforce visibility and synchronization.
            We do not use volatile for synchronization in C with pthreads.
            volatile is useful for memory-mapped I/O in embedded bare-metal (registers that can change outside program control). But not for thread synchronization in Linux/POSIX.

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

/* ----- Globals (shared between threads) ----- */

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
bool work_done = 0;

/* ----- Main Function ---- */

int main()
{
    printf("Main Thread (pid: %d, thread id: %lu) : Started.\n", getpid(), (unsigned long) pthread_self());

    pthread_t t1;

    // int retval = pthread_create(&t1, NULL, thread_function, "Arg Test");
    int val_to_pass = 5;
    int retval = pthread_create(&t1, NULL, thread_function, &val_to_pass);
    CHECK_ERR(retval, "Error creating thread");

    printf("Main Thread : Thread t1 is created successfully, waiting for t1's signal...\n");

    CHECK_ERR(pthread_mutex_lock(&mutex), "Lock failed"); // Lock mutex
    while (work_done == 0)
    {
        CHECK_ERR(pthread_cond_wait(&cond, &mutex), "Cond wait failed"); 
        // pthread_cond_wait atomically: releases mutex, blocks until signal arrives, re-locks mutex when it wakes up.
    }
    CHECK_ERR(pthread_mutex_unlock(&mutex), "Unlock failed"); // Unlock mutex

    printf("Main Thread : Detected that t1 finished it's work.\n");

    // We still use join to clean up thread resources

    void *thread_result;
    retval = pthread_join(t1, &thread_result);
    CHECK_ERR(retval, "Thread join failed");

    printf("Main Thread : t1 exited with status %ld\n", (long int) thread_result);
    printf("Main Thread : Exiting.\n");

    pthread_exit((void*)EXIT_SUCCESS);
}

/* ----- Function Implementations ---- */

static void *thread_function(void *arg)
{
    // char *passed_val = (char*) arg;
    int passed_val = *((int*) arg);

    printf("T1 (pid: %d, thread id: %lu) : Started executing\n", getpid(), (unsigned long) pthread_self());
    printf("T1 : Started doing work...\n");

    // printf("T1 : Passed arguments from main are: %s\n", passed_val);
    printf("T1 : Passed arguments from main are: %d\n", passed_val);
        
    // Lock, update shared state, and signal main

    CHECK_ERR(pthread_mutex_lock(&mutex), "Lock failed");
    work_done = 1;
    CHECK_ERR(pthread_cond_signal(&cond), "Signal failed");
    CHECK_ERR(pthread_mutex_unlock(&mutex), "Unlock failed");


    printf("T1 : Work done, signaled main. Exiting.\n");
    pthread_exit((void*)EXIT_SUCCESS);
}