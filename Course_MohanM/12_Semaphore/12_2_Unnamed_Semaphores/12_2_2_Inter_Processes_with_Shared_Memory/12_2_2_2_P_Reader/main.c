/* ---- NOTES ----
 *
 * Reader process:
 * - opens existing shared memory created by writer
 * - maps the same shared region structure
 * - waits until writer posts sem_data_ready
 * - reads the message
 * - posts sem_reader_done as acknowledgement
 *
 * This process does NOT call sem_destroy() or shm_unlink().
 * Those are owned by the creator/manager side in this design: the writer.
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

/* ---- Settings ---- */

#define SHARED_MEM_NAME "/shm1"
#define FILE_MODES      (O_RDWR)
#define FILE_PERMISSIONS 0

#define MESSAGE_MAX_LEN 256

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

/* ---- Shared Memory Layout ---- */

typedef struct shared_region
{
    sem_t sem_data_ready;
    sem_t sem_reader_done;
    char  message[MESSAGE_MAX_LEN];
} shared_region_t;

/* ---- Main ---- */

int main(void)
{
    int fd = -1;
    shared_region_t *shm_ptr = NULL;
    size_t len_shr = sizeof(shared_region_t);

    /* ---- Open existing shared memory ---- */
    fd = shm_open(SHARED_MEM_NAME, FILE_MODES, FILE_PERMISSIONS);
    if (fd == ERROR_GENERAL)
    {
        perror("R: shm_open");
        exit(EXIT_FAILURE);
    }

    printf("R: shm_open successful\n");

    /* ---- Map shared memory ---- */
    shm_ptr = mmap(VIRTUAL_MEM_ADDRESS_TO_BE_MAPPED,
                   len_shr,
                   SETTING_MEMORY_PROTECTION,
                   SETTING_VISIBILITY,
                   fd,
                   OFFSET_FD);

    if (shm_ptr == MAP_FAILED)
    {
        perror("R: mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    /* ---- fd no longer needed after mmap ---- */
    if (close(fd) == ERROR_GENERAL)
    {
        perror("R: close");
        munmap(shm_ptr, len_shr);
        exit(EXIT_FAILURE);
    }

    printf("R: fd closed\n");
    printf("R: Waiting for writer to signal that data is ready...\n");

    /* ---- Wait until writer says data is ready ---- */
    if (sem_wait(&shm_ptr->sem_data_ready) == ERROR_GENERAL)
    {
        perror("R: sem_wait sem_data_ready");
        munmap(shm_ptr, len_shr);
        exit(EXIT_FAILURE);
    }

    printf("R: Data is ready. Message received: \"%s\"\n", shm_ptr->message);

    /* ---- Notify writer that reader has consumed the data ---- */
    if (sem_post(&shm_ptr->sem_reader_done) == ERROR_GENERAL)
    {
        perror("R: sem_post sem_reader_done");
        munmap(shm_ptr, len_shr);
        exit(EXIT_FAILURE);
    }

    printf("R: Acknowledgement posted to writer\n");

    /* ---- Cleanup on reader side ---- */
    if (munmap(shm_ptr, len_shr) == ERROR_GENERAL)
    {
        perror("R: munmap");
        exit(EXIT_FAILURE);
    }

    printf("R: munmap done\n");
    printf("R: Exiting...\n");

    return EXIT_SUCCESS;
}