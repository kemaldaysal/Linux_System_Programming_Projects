/* ---- WARNINGS ---- 

* To Do: Synchronized IPC
    These scripts doesn't involve synchronized IPC, they only work safely if:

    -writer is run first
    -writer finishes writing before reader reads

    That is manual sequencing.

    For robust shared-memory IPC, we will add a coordination mechanism in later scripts. 
    That is exactly where inter-process unnamed semaphores should come in.

    Typical pattern will be:
    -shared memory contains payload + status fields
    -semaphore signals “data ready”
    -maybe another semaphore signals “slot free” or “reader consumed”
*/


/* ---- NOTES ----

* Shared memory allows to share a mapped memory region between unrelated processes.

* Shared memory is generally the fastest IPC method for bulk data exchange, compared to message queues, pipes, FIFO, because:
    * It avoids repeated copying between processes through kernel-mediated message passing.
        * In other IPC, if a data needs to be sent from P1 To P2, then there is 2 set of copies that is needed, 
          which are (sender process' userspace -> kernel space), (kernel space -> receiver process' userspace) 
        * Here, data is directly written to/read from the common shared portion of physical memory (RAM), 
          by mapping that memory address onto the current process' virtual memory.
    * Shared memory avoids repeated data movement through the kernel on each message transfer.

* Shared memory entries are present int '/dev/shm'

* When compiling the program manually, -lrt flag must be placed to compile it, 
    since -lrt is a linker instruction to include the POSIX Real-time Extensions library (librt) in the final executable  

* Shared memory name must be same between processes    
* When shared memory is created, it's size is usually 0 byte. 
    * We have to define the length of the shared memory, based on the data that we're planning to send
* Then map this shared memory onto the required processes virtual memories.
    * mmap() returns a pointer into the newly mapped memory. 
      This pointer points into the address space seen by the program and 
      can be used like any other pointer of type void*. On failure, mmap() returns MAP_FAILED.
    * Multiple processes mapping the same object see the same underlying memory
    * After mapping, pointer access is normal user-space memory access

* When a process finishes it's writing/listening job, 
    * It must close it's connection to shared memory location and 
    * Then deallocate that area with a specific size with nunmap function.
        * The OS will reclaim mappings on process exit, but for disciplined code we should still unmap explicitly.
        * munmap() removes the mapping from that process   
        * nunmap deallocates any mapping for the region starting at ADDR and extending LEN
          bytes. Returns 0 if successful, -1 for errors (and sets errno). 

    * shm_unlink() removes the name, but object can live until all references/mappings are gone.
        * This is created as a separate cleaner script for now, because this program is does not involve 'synchronized' IPC.

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
#define FILE_MODES (O_RDWR | O_CREAT)
#define FILE_PERMISSIONS (S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH)

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

/* ---- Main Function ---- */

int main(int argc, char *argv[])
{
    // ---- Input Message Validation
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <message>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // ---- Create a shared memory----
    int fd = shm_open(SHARED_MEM_NAME, FILE_MODES, FILE_PERMISSIONS);
    if (fd == ERROR_GENERAL)
    {
        perror("W: shm_open");
        exit(EXIT_FAILURE);
    }

    printf("W: SHM open success\n");

    // ----- Define the length of the shared memory based on the size of the data that will be passed ----
    size_t len_bytes = strlen(argv[1]) + 1; // +1 for including '\0' terminator
    if (ftruncate(fd, (off_t) len_bytes) == ERROR_GENERAL)
    {
        perror("W: Error (ftruncate)");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("W: Resized shared memory to %zu bytes\n", len_bytes);

    // ----- Map the shared memory onto the process' virtual memory
    
    char* addr = mmap(VIRTUAL_MEM_ADDRESS_TO_BE_MAPPED, len_bytes, SETTING_MEMORY_PROTECTION, SETTING_VISIBILITY, fd, OFFSET_FD);
    if (addr == MAP_FAILED)
    {
        perror("W: mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Now, any operation on the process' virtual memory, also writes the data onto the shared memory 

    // ----- Close fd since it is not needed anymore
    if (close(fd) == ERROR_GENERAL)
    {
        perror("W: Error closing fd");
        munmap(addr, len_bytes);
        exit(EXIT_FAILURE);
    }

    printf("W: fd closed\n");

    // Copying the data to the mapped memory location
    memcpy(addr, argv[1], len_bytes);

    printf("W: memcpy successful, data is copied to mapped memory location.\n");
    printf("W: New data at the address is: %s\n", addr);

    // ----- Deallocate when job is done in this side ----
    if (munmap(addr, len_bytes) == -1)
    {
        perror("W: munmap");
        exit(EXIT_FAILURE);
    }

    printf("W: Deallocation (nunmap) done on my side\n");

    printf("W: Exiting...\n");

    return EXIT_SUCCESS;
}