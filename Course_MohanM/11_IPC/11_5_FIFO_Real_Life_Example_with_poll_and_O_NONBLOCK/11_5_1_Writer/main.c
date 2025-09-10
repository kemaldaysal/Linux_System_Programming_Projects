/* ---- Headers ---- */
#define _POSIX_C_SOURCE 200809L // used to ignore SIG_PIPE
#include <signal.h>

#include <sys/stat.h> // for FIFO
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <poll.h>

/* ---- Enumerations and Defines ---- */

#define FILE_MODES (O_WRONLY | O_NONBLOCK) // fifo will be opened as write only in non-blocking moode
#define BUF_SIZE 256
#define FIFO_PATH "../fifo_channel"
#define TIMEOUT_POLL_IN_MS 100

/* ---- Globals ---- */

static int fd = -1;
volatile sig_atomic_t stop_flag = 0;

/* ---- Function Prototypes ---- */
void signal_handler(int sig);
void die(const char *msg);
void cleanup(void);

/* ---- Main Function ---- */

int main()
{
    printf("W (%d) : Started executing.\n", getpid());

    struct sigaction sa = {0};
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);

    struct sigaction sa_pipe = {0};
    sa_pipe.sa_handler = SIG_IGN;
    sigemptyset(&sa_pipe.sa_mask);
    sigaction(SIGPIPE, &sa_pipe, NULL);

    fd = open(FIFO_PATH, FILE_MODES);
    if (fd == -1)
    {
        die("W : Opening fifo failed");
    }

    printf("W : Opened FIFO, starting polling...\n");

    struct pollfd pfd = {.fd = fd, .events = POLLOUT};

    char buf_write[BUF_SIZE] = {0};
    while (!stop_flag)
    {
        if (poll(&pfd, 1, TIMEOUT_POLL_IN_MS) <= 0)
        {
            continue;
        }
        
        printf("W : Enter string to be sent via fifo:\n");
        if (!fgets(buf_write, BUF_SIZE, stdin))
        {
            die("W : fgets failed");
        }

        buf_write[strcspn(buf_write, "\n")] = '\0'; // Strip trailing newline from fgets for edge cases
        uint32_t len = strlen(buf_write) + 1;       // +1 to include '\0'

        // ----- First write : Message length

        ssize_t total_written = 0;
        while (total_written < sizeof(len))
        {
            ssize_t n = write(fd, (((uint8_t*)&len) + total_written), (sizeof(len) - total_written));
            if (n == -1)
            {
                if(errno == EINTR)
                {
                    if (stop_flag)
                    {
                        die("W : Signal caught mid-write");
                    }
                    continue;
                }
                else if (errno == EAGAIN)
                {
                    continue; // retry next poll
                }
                die("W : Failed to write length");
            }
            total_written += n;
        }

        // ----- Second write : Payload

        total_written = 0;

        while (total_written < len)
        {
            ssize_t n = write(fd, (buf_write + total_written), (len - total_written));

            if (n == -1)
            {
                if (errno == EPIPE)
                {
                    fprintf(stderr, "W : Broken pipe (no readers)\n");
                    die(NULL);
                }
                else if (errno == EINTR)
                {
                    if (stop_flag)
                    {
                        die("W : Signal caught mid-write");
                    }
                    fprintf(stderr, "W : Writing interrupted by a signal. Retrying the same write...");
                    continue;
                }
                else if (errno == EAGAIN)
                {
                    continue;
                }
                die("W : Error sending the buffer to fifo");
            }
            total_written += n;
        }
    }
    die("W : Signal caught");
}

void signal_handler(int sig)
{
    stop_flag = 1;
}

void die(const char *msg)
{
    if (msg)
    {
        perror(msg);
    }
    cleanup();
    printf("W : Exiting\n");
    exit(EXIT_FAILURE);
}

void cleanup(void)
{
    if (fd != -1)
    {
        close(fd);
        fd = -1;
    }
    printf("W : Cleaned up\n");
}