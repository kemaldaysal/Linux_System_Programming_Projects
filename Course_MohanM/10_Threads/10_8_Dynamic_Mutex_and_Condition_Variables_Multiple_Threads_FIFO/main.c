/* ----- Libraries ---- */

#define _POSIX_C_SOURCE 200112L // for timedlock
#define _GNU_SOURCE             // GNU extension for non-blocking join

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/* ----- Settings through defines ---- */

#define COUNT_OF_THREADS 3
#define READY_QUEUE_SIZE (COUNT_OF_THREADS + 1)

/* ----- Globals and Shared Variables ---- */

typedef struct
{
    pthread_mutex_t mutex;
    pthread_cond_t cond_to_t;
    pthread_cond_t cond_from_t;
    bool ready;
    bool to_proceed;
    int arg;
    int id;
} thread_data_t;

pthread_mutex_t mutex_g_sync = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_g_to_main = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_g_to_t = PTHREAD_COND_INITIALIZER;
int ready_queue[READY_QUEUE_SIZE];
size_t rq_head = 0;
size_t rq_tail = 0;
size_t rq_count = 0;

/* ----- Macro Functions ---- */

#define CHECK_ERR(ret)                              \
    do                                              \
    {                                               \
        if (ret != 0)                               \
        {                                           \
            fprintf(stderr, "%s\n", strerror(ret)); \
            exit(EXIT_FAILURE);                     \
        }                                           \
    } while (0)

/* ----- Function Prototypes ---- */

static void *thread_handler(void *arg);
static void ready_queue_push(int id);
static int ready_queue_pop(int *popped_id);

/* ----- Main Function ---- */

int main()
{
    printf("MT (tid: %lu, pid: %d): Started executing.\n", (unsigned long)pthread_self(), getpid());
    printf("MT : Creating %d threads.\n", COUNT_OF_THREADS);

    pthread_t t[COUNT_OF_THREADS];
    thread_data_t t_data[COUNT_OF_THREADS];
    memset(t_data, 0, sizeof(t_data));

    for (size_t i = 0; i < COUNT_OF_THREADS; i++)
    {
        CHECK_ERR(pthread_mutex_init(&(t_data[i].mutex), NULL));
        CHECK_ERR(pthread_cond_init(&(t_data[i].cond_from_t), NULL));
        CHECK_ERR(pthread_cond_init(&(t_data[i].cond_to_t), NULL));
        t_data[i].ready = t_data[i].to_proceed = 0;
        t_data[i].arg = 100 + i;
        t_data[i].id = i;
        CHECK_ERR(pthread_create(&t[i], NULL, thread_handler, &(t_data[i])));
        // pthread_detach(t[i]);
    }

    size_t count_of_ready_threads = 0;

    while (count_of_ready_threads < COUNT_OF_THREADS)
    {
        CHECK_ERR(pthread_mutex_lock(&mutex_g_sync));
        while (rq_head == rq_tail)
        {
            CHECK_ERR(pthread_cond_wait(&(cond_g_to_main), &mutex_g_sync));
        }

        int id;
        while (ready_queue_pop(&id) != 0)
        {
            CHECK_ERR(pthread_mutex_unlock(&mutex_g_sync));
            CHECK_ERR(pthread_mutex_lock(&(t_data[id].mutex)));
            if (!t_data[id].to_proceed)
            {
                printf("MT : For T%d, got the ready signal\n", t_data[id].id);
                ++count_of_ready_threads;
                t_data[id].to_proceed = 1;
                CHECK_ERR(pthread_cond_signal(&(t_data[id].cond_to_t)));
                printf("MT : For T%d, sent the proceed signal\n", t_data[id].id);
            }
            CHECK_ERR(pthread_mutex_unlock(&(t_data[id].mutex)));
            CHECK_ERR(pthread_mutex_lock(&mutex_g_sync));
        }
        CHECK_ERR(pthread_mutex_unlock(&mutex_g_sync));
    }

    size_t finished_threads = 0;
    while (finished_threads < COUNT_OF_THREADS)
    {
        for (size_t i = 0; i < COUNT_OF_THREADS; i++)
        {
            if (t[i] == 0)
            {
                continue; // thread has finished, continue to next step of for loop
            }

            void *status;
            int rc = pthread_tryjoin_np(t[i], &status);
            if (rc == 0)
            {
                printf("MT : t(%lu) finished, cleaning up\n", i);
                t[i] = 0;
                finished_threads++;
                CHECK_ERR(pthread_mutex_destroy(&(t_data[i].mutex)));      // IS THIS NECESSARY??
                CHECK_ERR(pthread_cond_destroy(&(t_data[i].cond_to_t)));   // IS THIS NECESSARY??
                CHECK_ERR(pthread_cond_destroy(&(t_data[i].cond_from_t))); // IS THIS NECESSARY??
            }
        }
        usleep(100000); // to avoid busy-spin (100ms)
    }

    printf("MT : Exiting.\n");
    pthread_exit((void *)EXIT_SUCCESS);
}

/* ----- Function Implementations ---- */

static void *thread_handler(void *arg)
{
    thread_data_t *t_data = arg;

    printf("-T%d (tid: %lu) : Started executing...\n", t_data->id, (unsigned long)pthread_self());

    switch (t_data->id)
    {
    case 0:
        sleep(10);
        break;

    case 1:
        sleep(5);
        break;

    case 2:
        sleep(1);
        break;

    default:
        break;
    }

    CHECK_ERR(pthread_mutex_lock(&(t_data->mutex)));
    t_data->ready = 1;
    CHECK_ERR(pthread_cond_signal(&(t_data->cond_from_t)));
    CHECK_ERR(pthread_mutex_unlock(&(t_data->mutex)));

    CHECK_ERR(pthread_mutex_lock(&mutex_g_sync));
    ready_queue_push(t_data->id);
    CHECK_ERR(pthread_cond_signal(&cond_g_to_main));
    CHECK_ERR(pthread_mutex_unlock(&mutex_g_sync));

    printf("-T%d : Sent ready to main (via global queue), waiting for proceed signal\n", t_data->id);

    CHECK_ERR(pthread_mutex_lock(&(t_data->mutex)));
    while ((t_data->to_proceed) != 1)
    {
        CHECK_ERR(pthread_cond_wait(&(t_data->cond_to_t), &(t_data->mutex)));
    }
    printf("-T%d : Got the proceed signal from main\n", t_data->id);
    CHECK_ERR(pthread_mutex_unlock(&(t_data->mutex)));

    printf("-T%d : Passed argument is %d\n", t_data->id, t_data->arg);
    printf("-T%d : Finished, exiting...\n", t_data->id);

    pthread_exit((void *)EXIT_SUCCESS);
}

static void ready_queue_push(int id)
{
    size_t next = (rq_tail + 1) % READY_QUEUE_SIZE;
    if (next == rq_head)
    {
        fprintf(stderr, "ready queue overflow\n");
        return;
    }
    ready_queue[rq_tail] = id;
    rq_tail = next;

    printf("[PUSH] id=%d  rq_head=%zu  rq_tail=%zu\n",
           id, rq_head, rq_tail);
}

static int ready_queue_pop(int *popped_id)
{
    if (rq_head == rq_tail) // queue is empty
    {
        return 0;
    }

    *popped_id = ready_queue[rq_head];
    rq_head = (rq_head + 1) % READY_QUEUE_SIZE;

    printf("[POP ] id=%d  rq_head=%zu  rq_tail=%zu\n", *popped_id, rq_head, rq_tail);

    return 1;
}