#include <sys/stat.h> // for FIFO
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

#define FILE_MODES (O_RDONLY) // fifo will be opened as read only mode
#define FIFO_PATH "../my_fifo"

#define BUF_SIZE 80

int main()
{
    printf("R (pid: %d): Started executing\n", getpid());

    printf("R : I'll open FIFO and block for writers to arrive.\n");

    char *my_fifo = FIFO_PATH;
    int fd = open(my_fifo, FILE_MODES);
    if (fd == -1)
    {
        perror("R : Opening FIFO failed.");
        exit(EXIT_FAILURE);
    }

    printf("R : Writers arrived, proceeding...\n");

    while (1)
    {
        // ----- First read : message length

        uint32_t msg_len = 0;
        ssize_t n = read(fd, &msg_len, sizeof(msg_len));
        if (n != sizeof(msg_len))
        {
            if (n == 0)
            {
                printf("R : Writer closed FIFO\n");
                // break;
            }
            else
            {
                perror("R : Failed to read message length");
                break;
            }
        }

        if (msg_len <= 0 || msg_len > BUF_SIZE)
        {
            printf("R : Invalid message length %u\n", msg_len);
            break;
        }

        // ----- Second read : message

        char *buf_read = malloc(msg_len + 1);
        if (!buf_read)
        {
            perror("R : Malloc failed");
            break;
        }

        ssize_t total_read = 0;
        int error = 0;

        while (total_read < msg_len)
        {
            n = read(fd, buf_read + total_read, msg_len - total_read);
            if (n == -1)
            {
                if (errno == EINTR)
                {
                    perror("Reading interrupted by a signal, continuing to read");
                    continue;
                }
                perror("R : Reading error\n");
                error = 1;
                goto cleanup;
            }
            else if (n == 0)
            {
                printf("R : Writer closed the FIFO mid-message\n");
                error = 1;
                goto cleanup;
            }
            total_read += n;
        }

        if (total_read == msg_len)
        {
            buf_read[msg_len] = '\0'; // to ensure null-termination
            printf("R : Writer sent: %s\n", buf_read);
        }
        else
        {
            printf("R : ERROR: Couldn't read all bits\n");
        }

    cleanup:
        free(buf_read);
        buf_read = NULL;

        if (error == 1)
            continue;
    }

    printf("R : Closing fd\n");
    unlink(my_fifo);
    close(fd); // ASK HOW TO CLEANUP THE FIFO? DOES THE FIFO BE REMOVED AND MADE INACTIVE AFTER PROCESSES USING IT TERMINATES?
    printf("R : Exiting\n");
    exit(EXIT_SUCCESS);
}