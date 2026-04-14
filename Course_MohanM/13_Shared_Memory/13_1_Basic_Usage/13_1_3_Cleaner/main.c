#define _XOPEN_SOURCE 700
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>

#define SHARED_MEM_NAME "/shm1"

int main(void)
{
    if (shm_unlink(SHARED_MEM_NAME) == -1)
    {
        perror("cleanup: shm_unlink");
        return EXIT_FAILURE;
    }

    printf("cleanup: shared memory unlinked\n");
    return EXIT_SUCCESS;
}