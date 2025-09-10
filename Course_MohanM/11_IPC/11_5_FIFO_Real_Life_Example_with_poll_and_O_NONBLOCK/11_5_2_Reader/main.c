/* ---- Notes ----

Features:


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
#include <poll.h> // for poll

/* ---- Enumerations and Defines ---- */

#define FILE_PERMISSIONS (S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH)
#define FILE_MODES (O_RDONLY | O_NONBLOCK) // fifo will be opened as read only mode
#define FIFO_PATH "../fifo_channel"
#define BUF_SIZE 256
#define TIMEOUT_POLL_IN_MS 100

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

    printf("R : Opening FIFO in non-blocking mode...\n");

    fd = open(FIFO_PATH, FILE_MODES);
    if (fd == -1)
    {
        die("R : Opening FIFO failed", NULL);
    }

    printf("R : FIFO opened, proceeding...\n");

    struct pollfd pfd =
        {
            .fd = fd,
            .events = POLLIN // We only care about "data ready to read"
        };

    while (!stop_flag)
    {

        int ret = poll(&pfd, 1, TIMEOUT_POLL_IN_MS);
        if (ret == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            die("R : poll failed", NULL);
        }
        else if (ret == 0)
        {
            continue; // timeout, no data, loop again
        }

        if (pfd.revents & POLLIN)
        {

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
                    else if (errno == EAGAIN)
                    {
                        // Don’t lose progress, just retry in the same loop
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

            if (total_read < sizeof(msg_len))
            {
                continue; // incomplete length, wait
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
                    else if (errno == EAGAIN)
                    {
                        // Don’t lose progress, just retry in the same loop
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

            if (total_read < msg_len)
            {
                free(buf_read);
                continue; // incomplete message, wait next poll
            }

            buf_read[msg_len] = '\0'; // to ensure null-termination
            printf("R : Writer sent: %s\n", buf_read);
            free(buf_read);
        }
    }
    die("R : Signal caught", NULL);
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