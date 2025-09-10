/* ---- Notes ----

Features:
    -All the system calls and regular file operations can also be used on FIFO

Opening and Creation:
    -If you open a FIFO in write-only mode (O_WRONLY), the open() blocks until some other process opens the FIFO in read mode (O_RDONLY).
    -If no reader exists, your program will appear to "hang" at open() — this is exactly what you’re observing.
    -If you don't want it to hang/block, you can put O_NONBLOCK into FILE_MODES.

    -mkfifo() fails if the FIFO file already exists. That’s normal. You can safely ignore this error if your intention is to reuse the same FIFO.

Unlinking:
    -mkfifo() creates a special file in the filesystem. It does not disappear when processes exit.
    If you don’t unlink() it, it stays there as a named pipe file. We must unlink() the FIFO path either on program exit.
    -Only one process should be responsible for creating/unlinking the FIFO.

    -If both writer and reader call unlink, that's risky since:
    If the writer exits early, it deletes the FIFO while the reader is still using it.
    Keep cleanup in only the reader (or whichever side you designate as the “owner”).

Roles:
    Server (Reader)
        -In usual convention of server-client model, reader (server) calls mkfifo() at startup to ensure FIFO exists. 
        -At shutdown, server unlink() at shutdown to clean up FIFO.
        -The server process owns the communication channel. Clients (writers) come and go, but the server controls the resource.
    
    Client (Writer(s))
        -Only open()s the FIFO (no creation, no unlink).
        -A client should not mess with the shared resource.

    - This avoids races. If a writer also unlinks the FIFO, it could delete it while other writers are still using it.

Design Alternatives:
    - If your design is long-lived reader, short-lived writers → let reader create + unlink.
    - If your design is long-lived writer, short-lived readers → writer could own it.
    But in 99% of IPC setups, the reader/server owns the FIFO, because the reader is the stable process.

*/

/* ---- Headers ---- */
#define _POSIX_C_SOURCE 200809L
#include <sys/stat.h> // for FIFO
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <signal.h>

/* ---- Enumerations and Defines ---- */

#define FILE_PERMISSIONS (S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH)
#define FILE_MODES (O_RDONLY) // fifo will be opened as read only mode
#define FIFO_PATH "../fifo_channel"
#define BUF_SIZE 256

/* ---- Globals ---- */

static int fd = -1;
volatile sig_atomic_t stop_flag = 0;

/* ---- Function Prototypes ---- */
void signal_handler(int sig);
void die(const char *msg, char *buf);
void cleanup(void);

/* ---- Main Function ---- */

int main()
{
    printf("R (pid: %d): Started executing\n", getpid());

    struct sigaction sa = {0};
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);

    if (mkfifo(FIFO_PATH, FILE_PERMISSIONS) == -1)
    {
        if (errno == EEXIST) // to handle the error for existing same fifo and reuse the same FIFO
        {
            printf("R : Reusing the existing FIFO already.\n");
        }
        else
        {
            die("R : Creating fifo failed", NULL);
        }
    }
    else
    {
        printf("R : FIFO created successfully.\n");
    }

    printf("R : Opening FIFO, blocking until a writer connects...\n");

    fd = open(FIFO_PATH, FILE_MODES);
    if (fd == -1)
    {
        die("R : Opening FIFO failed", NULL);
    }

    printf("R : Writer connected, proceeding...\n");

    while (1)
    {
        if (stop_flag)
        {
            die("R : Signal caught", NULL);
        }

        // ----- First read : message length

        uint32_t msg_len = 0;
        uint8_t *len_ptr = (uint8_t *)&msg_len;
        size_t total_read = 0;

        while (total_read < sizeof(msg_len))
        {
            ssize_t n = read(fd, len_ptr + total_read, sizeof(msg_len) - total_read);

            if (n == -1)
            {
                if (errno == EINTR)
                {
                    if (stop_flag)
                    {
                        die("R : Signal caught mid-read.", NULL);
                    }
                    fprintf(stderr, "R : Read interrupted by a signal, retrying...\n");
                    continue;
                }
                die("R : Failed to read length", NULL);
            }
            else if (n == 0)
            {
                printf("R : Writer closed FIFO\n");
                cleanup();
                exit(EXIT_SUCCESS);
            }
            total_read += n;
        }

        if (msg_len == 0 || msg_len > BUF_SIZE)
        {
            fprintf(stderr, "R : Invalid message length %u\n", msg_len);
            die(NULL, NULL);
        }

        // ----- Second read : message

        char *buf_read = malloc(msg_len + 1);
        if (!buf_read)
        {
            die("Malloc failed", buf_read);
        }

        total_read = 0;
        while (total_read < msg_len)
        {
            if (stop_flag)
            {
                die("R : Signal caught mid-read", buf_read);
            }

            ssize_t n = read(fd, (buf_read + total_read), (msg_len - total_read));
            if (n == -1)
            {
                if (errno == EINTR)
                {
                    if (stop_flag)
                    {
                        die("R : Signal caught mid-read", buf_read);
                    }
                    fprintf(stderr, "R : Read interrupted, retrying...\n");
                    continue;
                }
                die("R : Error reading payload", buf_read);
            }
            else if (n == 0)
            {
                fprintf(stderr, "R : Writer closed FIFO mid-message\n");
                die(NULL, buf_read);
            }
            total_read += n;
        }

        buf_read[msg_len] = '\0'; // to ensure null-termination
        printf("R : Writer sent: %s\n", buf_read);
        free(buf_read);
    }
}

void signal_handler(int sig)
{
    stop_flag = 1;
}

void die(const char *msg, char *buf)
{
    if (msg)
    {
        perror(msg);
    }
    if (buf)
    {
        free(buf);
    }
    cleanup();
    printf("R : Exiting\n");
    exit(EXIT_FAILURE);
}

void cleanup(void)
{
    if (fd != -1)
    {
        close(fd);
        fd = -1;
    }
    unlink(FIFO_PATH); // only reader must remove FIFO
    printf("R : Cleaned up FIFO\n");
}