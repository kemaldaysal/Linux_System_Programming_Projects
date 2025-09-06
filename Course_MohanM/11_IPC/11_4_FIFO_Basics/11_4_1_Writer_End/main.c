/* NOTES
-All the system calls and regular file operations can also be used on FIFO

-If you open a FIFO in write-only mode (O_WRONLY), the open() blocks until some other process opens the FIFO in read mode (O_RDONLY).
-If no reader exists, your program will appear to "hang" at open() — this is exactly what you’re observing.
-If you don't want it to hang/block, you can put O_NONBLOCK into FILE_MODES.

-mkfifo() fails if the FIFO file already exists. That’s normal. You can safely ignore this error if your intention is to reuse the same FIFO.

-mkfifo()creates a special file in the filesystem. It does not disappear when processes exit.
If you don’t unlink() it, it stays there as a named pipe file. We must unlink() the FIFO path either on program exit.
Only one process should be responsible for creating/unlinking the FIFO.
Usually the server (reader) does mkfifo() on startup and unlink() on shutdown.
Clients (writers) just open() the existing FIFO, never unlink it.
If both writer and reader call unlink, that's risky since:
    If the writer exits early, it deletes the FIFO while the reader is still using it.
    Keep cleanup in only the reader (or whichever side you designate as the “owner”).
*/

/* ---- Headers ---- */
#define _POSIX_C_SOURCE 200809L // used to ignore SIG_PIPE
#include <signal.h>

#include <sys/stat.h> // for FIFO
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

/* ---- Enumerations and Defines ---- */

#define FILE_PERMISSIONS (S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH)
#define FILE_MODES (O_WRONLY) // fifo will be opened as write only mode

#define BUF_SIZE 80
#define FIFO_PATH "../my_fifo"

/* ---- Main Function ---- */

int main()
{
    printf("W (%d) : Started executing.\n", getpid());

    signal(SIGPIPE, SIG_IGN); // sigaction must be used since signal is obsolete, only for demonstration!!

    char *my_fifo = FIFO_PATH;

    if (mkfifo(my_fifo, FILE_PERMISSIONS) == -1)
    {
        if (errno != EEXIST) // to handle the error for existing same fifo and reuse the same FIFO
        {
            perror("W : Creating fifo failed.\n");
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("FIFO already exists, using the existing one.\n");
        }
    }

    printf("W : FIFO (WRONLY) created successfully.\n");
    printf("W : Opening FIFO, if no readers present, i'll block here until they exist.\n");

    int fd = open(my_fifo, FILE_MODES);
    if (fd == -1)
    {
        perror("W : Opening fifo failed\n");
        exit(EXIT_FAILURE);
    }

    printf("W : Opened FIFO since there are listeners present.\n");

    bool done = 0;
    char buf_rw[BUF_SIZE] = {'\0'};
    while (done != 1)
    {
        printf("W : Enter string to be sent via fifo:\n");
        if (!fgets(buf_rw, BUF_SIZE, stdin))
        {
            perror("Error writing stdin to buffer");
            break;
        }

        uint32_t len = strlen(buf_rw) + 1; // +1 to include '\0'

        // ----- First write : Message length

        ssize_t n = write(fd, &len, sizeof(len));
        if (n != sizeof(len))
        {
            perror("W : Failed to write length");
            break;            
        }

        // ----- Second write : Message

        ssize_t total_written = 0;

        while (total_written < len)
        {
            ssize_t n = write(fd, buf_rw + total_written, len - total_written);

            if (n == -1)
            {
                if (errno == EPIPE)
                {
                    printf("W : Reader closed FIFO, write returned EPIPE.\n");
                    printf("W : Since SIG_PIPE signal is ignored, the program will continue\n");
                    done = 1;
                    break;
                }
                else if (errno == EINTR)
                {
                    perror("Reading interrupted by a signal, continuing to read");
                    continue;
                }
                else
                {
                    perror("W : Error writing buffer to fd");
                    break;
                }
            }
            total_written += n;
        }
    }
    printf("W : Closing fd\n");
    unlink(my_fifo);
    close(fd);
    printf("W : Exiting\n");

    exit(EXIT_SUCCESS);
}