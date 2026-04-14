/* ---- NOTES ----
 *
 * This example demonstrates:
 * 1) POSIX shared memory
 * 2) Unnamed semaphores stored INSIDE shared memory
 * 3) A two-process handshake using two semaphores
 *
 * Handshake:
 *   Writer:
 *     - creates shared memory
 *     - initializes semaphores
 *     - writes message
 *     - posts sem_data_ready
 *     - waits on sem_reader_done
 *     - destroys semaphores and unlinks shared memory
 *
 *   Reader:
 *     - opens existing shared memory
 *     - waits on sem_data_ready
 *     - reads message
 *     - posts sem_reader_done
 *
 * Important:
 * - For process-shared unnamed semaphores:
 *     1) pshared must be nonzero in sem_init()
 *     2) sem_t object must live in shared memory
 *
 * - Do NOT create a local sem_t variable and try to memcpy it into shared memory.
 *   The semaphore must be constructed directly inside the shared mapping.
 *
 */

#define _XOPEN_SOURCE 700

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <errno.h>

/* ---- Settings ---- */

#define MESSAGE "Hello World"
#define SHARED_MEM_NAME "/shm1"

#define FILE_MODES        (O_RDWR | O_CREAT | O_EXCL)
#define FILE_PERMISSIONS  (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)

#define MESSAGE_MAX_LEN   256

#define SVAL_INITIAL_DATA_READY   0
#define SVAL_INITIAL_READER_DONE  0

#define VIRTUAL_MEM_ADDRESS_TO_BE_MAPPED NULL
#define SETTING_MEMORY_PROTECTION (PROT_READ | PROT_WRITE) // R & W Protected
#define SETTING_VISIBILITY MAP_SHARED // shared memory data is visible to all other processes
#define OFFSET_FD 0 

/* ---- Enumerations ---- */

enum RETURN_TYPES
{
    SUCCESS = 0,
    ERROR_GENERAL = -1
};

enum PSHARED_SETTING
{
    PSHARED_BETWEEN_THREADS   = 0,
    PSHARED_BETWEEN_PROCESSES = 1
};

/* ---- Shared Memory Layout ---- */

typedef struct shared_region
{
    sem_t sem_data_ready;   /* writer posts, reader waits */
    sem_t sem_reader_done;  /* reader posts, writer waits */
    char  message[MESSAGE_MAX_LEN];
} shared_region_t;

/* ---- Main ---- */

int main(int argc, char *argv[])
{
    int fd = -1;
    shared_region_t *shm_ptr = NULL;
    size_t len_shr = sizeof(shared_region_t);
    const char *msg_to_send = NULL;

    msg_to_send = MESSAGE;

    if (strlen(msg_to_send) >= MESSAGE_MAX_LEN)
    {
        fprintf(stderr,
                "W: Message too long. Max allowed length is %d bytes.\n",
                MESSAGE_MAX_LEN - 1);
        exit(EXIT_FAILURE);
    }

    /* ---- Create shared memory object ---- */
    fd = shm_open(SHARED_MEM_NAME, FILE_MODES, FILE_PERMISSIONS);
    if (fd == ERROR_GENERAL)
    {
        perror("W: shm_open");
        exit(EXIT_FAILURE);
    }

    printf("W: shm_open successful\n");

    /* ---- Size the shared memory object ---- */
    if (ftruncate(fd, (off_t)len_shr) == ERROR_GENERAL)
    {
        perror("W: ftruncate");
        close(fd);
        shm_unlink(SHARED_MEM_NAME);
        exit(EXIT_FAILURE);
    }

    printf("W: Shared memory resized to %zu bytes\n", len_shr);

    /* ---- Map shared memory ---- */
    shm_ptr = mmap(VIRTUAL_MEM_ADDRESS_TO_BE_MAPPED,
                   len_shr,
                   SETTING_MEMORY_PROTECTION,
                   SETTING_VISIBILITY,
                   fd,
                   OFFSET_FD);

    if (shm_ptr == MAP_FAILED)
    {
        perror("W: mmap");
        close(fd);
        shm_unlink(SHARED_MEM_NAME);
        exit(EXIT_FAILURE);
    }

    /* ---- fd no longer needed after mmap ---- */
    if (close(fd) == ERROR_GENERAL)
    {
        perror("W: close");
        munmap(shm_ptr, len_shr);
        shm_unlink(SHARED_MEM_NAME);
        exit(EXIT_FAILURE);
    }

    printf("W: fd closed\n");

    /* ---- Initialize unnamed semaphores INSIDE shared memory ---- */
    if (sem_init(&shm_ptr->sem_data_ready,
                 PSHARED_BETWEEN_PROCESSES,
                 SVAL_INITIAL_DATA_READY) == ERROR_GENERAL)
    {
        perror("W: sem_init sem_data_ready");
        munmap(shm_ptr, len_shr);
        shm_unlink(SHARED_MEM_NAME);
        exit(EXIT_FAILURE);
    }

    if (sem_init(&shm_ptr->sem_reader_done,
                 PSHARED_BETWEEN_PROCESSES,
                 SVAL_INITIAL_READER_DONE) == ERROR_GENERAL)
    {
        perror("W: sem_init sem_reader_done");
        sem_destroy(&shm_ptr->sem_data_ready);
        munmap(shm_ptr, len_shr);
        shm_unlink(SHARED_MEM_NAME);
        exit(EXIT_FAILURE);
    }

    printf("W: Semaphores initialized successfully\n");

    /* ---- Write message to shared memory ---- */
    snprintf(shm_ptr->message, sizeof(shm_ptr->message), "%s", msg_to_send);

    printf("W: Message written to shared memory: \"%s\"\n", shm_ptr->message);

    /* ---- Notify reader that data is ready ---- */
    if (sem_post(&shm_ptr->sem_data_ready) == ERROR_GENERAL)
    {
        perror("W: sem_post sem_data_ready");
        sem_destroy(&shm_ptr->sem_reader_done);
        sem_destroy(&shm_ptr->sem_data_ready);
        munmap(shm_ptr, len_shr);
        shm_unlink(SHARED_MEM_NAME);
        exit(EXIT_FAILURE);
    }

    printf("W: sem_data_ready posted, waiting for reader acknowledgement...\n");

    /* ---- Wait for reader acknowledgement ---- */
    if (sem_wait(&shm_ptr->sem_reader_done) == ERROR_GENERAL)
    {
        perror("W: sem_wait sem_reader_done");
        sem_destroy(&shm_ptr->sem_reader_done);
        sem_destroy(&shm_ptr->sem_data_ready);
        munmap(shm_ptr, len_shr);
        shm_unlink(SHARED_MEM_NAME);
        exit(EXIT_FAILURE);
    }

    printf("W: Reader acknowledged successful read\n");

    /* ---- Cleanup ---- */
    if (sem_destroy(&shm_ptr->sem_reader_done) == ERROR_GENERAL)
    {
        perror("W: sem_destroy sem_reader_done");
    }

    if (sem_destroy(&shm_ptr->sem_data_ready) == ERROR_GENERAL)
    {
        perror("W: sem_destroy sem_data_ready");
    }

    if (munmap(shm_ptr, len_shr) == ERROR_GENERAL)
    {
        perror("W: munmap");
        shm_unlink(SHARED_MEM_NAME);
        exit(EXIT_FAILURE);
    }

    printf("W: munmap done\n");

    if (shm_unlink(SHARED_MEM_NAME) == ERROR_GENERAL)
    {
        perror("W: shm_unlink");
        exit(EXIT_FAILURE);
    }

    printf("W: shm_unlink done\n");
    printf("W: Exiting...\n");

    return EXIT_SUCCESS;
}