/* ----- Notes -----
    -By default, a thread is joinable, which means when a joinable thread terminates, 
    another thread is responsible to use pthread_join() to free up terminated thread's resources to 
    avoid it to become a zombie thread and leaks memory, 
    -And also another thread can obtain terminated thread's return value using pthread_join().
    -With pthread_detach(), system automatically cleans up resources when thread terminates.
    So no need to call pthread_join() anymore. 
    -But with this way, we can't obtain terminated thread's return values.  
*/

/* ----- Libraries -----*/

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/* ----- Macros -----*/

#define CHECK_ERR(err, msg) \
    do { \
        if (err != 0) { \
            fprintf(stderr, "%s: %s\n", msg, strerror(err)); \
            exit(EXIT_FAILURE); \
        } \
    } while(0) \

/* ----- Function Prototypes -----*/

static void *thread_function(void *arg);

/* ----- Main Function -----*/

int main()
{
    printf("MT (pid = %d, tid = %ld) : Started executing...\n", getpid(), (unsigned long) pthread_self());
    pthread_t t1;
    CHECK_ERR(pthread_create(&t1, NULL, thread_function, NULL), "Error creating the thread");
    CHECK_ERR(pthread_detach(t1), "Error setting thread as detachable"); // Set created thread as detachable

    pthread_exit((void*) EXIT_SUCCESS);
}

/* ----- Function Implementations -----*/

static void *thread_function(void *arg)
{
    printf("-T1 (tid = %ld) : Started executing...\n", (unsigned long) pthread_self());

    // pthread_detach(pthread_self()); // It can also set itself as detachable
    pthread_exit((void*) EXIT_SUCCESS);
}