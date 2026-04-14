/* ---- NOTES -----
* "struct stat" is a C data structure used in POSIX-compliant operating systems to store metadata about a file or directory.
    * It is populated by system calls such as stat(), fstat(), and lstat(). The structure is defined in the <sys/stat.h>
    * It contains members like st_dev, st_uid, st_gid etc.
*/

/* ---- Libraries ---- */

#define _XOPEN_SOURCE 500 

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

/* ---- Settings ---- */

#define SHARED_MEM_NAME "/shm1"
#define FILE_MODES (O_RDONLY)
#define FILE_PERMISSIONS 0

#define VIRTUAL_MEM_ADDRESS_TO_BE_MAPPED NULL
#define SETTING_MEMORY_PROTECTION (PROT_READ) // R & W Protected
#define SETTING_VISIBILITY MAP_SHARED // shared memory data is visible to all other processes
#define OFFSET_FD 0 

/* ---- Enumerations ---- */

enum RETURN_TYPES
{
    SUCCESS = 0,
    ERROR_GENERAL = -1
};

/* ---- Main Function ---- */

int main(int argc, char *argv[])
{
    // ---- Open an existing shared memory----
    int fd = shm_open(SHARED_MEM_NAME, FILE_MODES, FILE_PERMISSIONS);
    if (fd == ERROR_GENERAL)
    {
        perror("R: shm_open");
        exit(EXIT_FAILURE);
    }

    printf("R: SHM open success\n");

    // ---- Get file attributes for the file, device, pipe, or socket that file descriptor FD is open on and put them in BUF.
    struct stat file_attr;
    if (fstat(fd, &file_attr) == ERROR_GENERAL)
    {
        perror("R: fstat");
        close(fd);
        exit(EXIT_FAILURE);        
    } 

    if (file_attr.st_size <= 0)
    {
        fprintf(stderr, "R: shared memory size is invalid: %ld\n", (long)file_attr.st_size);
        close(fd);
        exit(EXIT_FAILURE);
    }

    size_t len_bytes = (size_t)file_attr.st_size;

    // ----- Map the shared memory onto the process' virtual memory

    char* addr = mmap(VIRTUAL_MEM_ADDRESS_TO_BE_MAPPED, len_bytes, SETTING_MEMORY_PROTECTION, SETTING_VISIBILITY, fd, OFFSET_FD);
    if (addr == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("R: mmap successfull, read data at the address is: %s\n", addr);

    // ----- Close fd since it is not needed anymore
    if (close(fd) == ERROR_GENERAL)
    {
        perror("R: Error closing fd");
        munmap(addr, len_bytes);
        exit(EXIT_FAILURE);
    }        

    printf("R: fd closed\n");

    if (munmap(addr, len_bytes) == -1)
    {
        perror("R: munmap");
        exit(EXIT_FAILURE);
    }    

    printf("R: Deallocation (nunmap) done on my side\n");

    printf("R: Exiting...\n");
    return(EXIT_SUCCESS);
}