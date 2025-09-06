/*  NOTES

    -If the pipe has no data but at least one writer still has the pipe open, then read() blocks until either data is written or the write end is closed.
    So even when the parent sleeps, the child is perfectly happy to sit there “listening”.
    It isn’t racing to grab something that isn’t there yet. It simply blocks, thanks to blocking semantics of read() on a pipe.
    Because the child is blocked in read(), as soon as the parent writes, the kernel wakes the child up and delivers the data.

    -A pipe isn’t like a global variable in memory that one process might read before another writes.
        -A pipe has built-in synchronization semantics. A read() call on an empty pipe does not “miss” data; it just blocks until either:
            -Data arrives (then it returns the data), or
            -All writers close the pipe (then it returns 0 = EOF).

    -So even though the parent “comes late,” the child just waits.
    Our “race condition” intuition would apply if we had a non-blocking read (O_NONBLOCK) or were working with a regular memory buffer. But with pipes, blocking I/O handles this synchronization for us.
*/

/* ---- Headers ---- */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <wait.h>

/* ---- Enumerations and Defines ---- */

enum FD_PIPE_ENDS
{
    READING_END = 0,
    WRITING_END = 1
};

enum SIZE_OF_BUFFERS
{
    SIZE_OF_BUF_READ = 64,
    SIZE_OF_BUF_WRITE = 64
};

#define BYTES_TO_READ_AT_ONCE 8
#define SLEEP_TIME 2

/* ---- Main Function ---- */

int main()
{
    printf("P (%d) : Started executing...\n", getpid());

    int fd_pipe[2] = {0};

    if (pipe(fd_pipe) == -1)
    {
        perror("P : Error creating pipe\n");
        exit(EXIT_FAILURE);
    }

    printf("P : Pipe is created.\n");

    pid_t cpid = fork();
    switch (cpid)
    {
    case -1:
        perror("P : Error forking\n");
        exit(EXIT_FAILURE);
        break;

    case 0:
        printf("-C (pid: %d) : Started executing...\n", getpid());

        printf("-C : Closing writing end of the pipe on my side, since i won't use it\n");
        if (close(fd_pipe[WRITING_END]) == -1) // child won't write, only listens
        {
            printf("-C : Error closing fd[%d]\n", WRITING_END);
            exit(EXIT_FAILURE);
        }

        printf("-C : Starting to read the pipe...\n");

        char buf_read[SIZE_OF_BUF_READ] = {'\0'};

        while (1)
        {
            ssize_t bytes_read = read(fd_pipe[READING_END], buf_read, BYTES_TO_READ_AT_ONCE);
            if (bytes_read == -1)
            {
                printf("-C : Error reading the pipe\n");
                exit(EXIT_FAILURE);
            }
            else if (bytes_read == 0)
            {
                printf("-C : Parent closed write end of the pipe\n");
                printf("-C : I'm also closing the reading end now.\n");
                if (close(fd_pipe[READING_END]) == -1)
                {
                    printf("-C : Error closing the reading end of the pipe\n");
                    exit(EXIT_FAILURE);
                }
                break;
            }
            else
            {
                buf_read[bytes_read] = '\0';
                printf("-C : Data read: %s\n", buf_read);
            }
        }

        printf("-C : Exiting...\n");
        exit(EXIT_SUCCESS);

    default:
        printf("P : Child is forked.\n");
        break;
    }

    printf("P : Closing the reading end of the pipe on my side, since i won't use it.\n");
    if (close(fd_pipe[READING_END]) == -1)
    {
        printf("P : Error closing the reading end of the pipe\n");
    }

    printf("P : Now i'll sleep to show blocking behaviour of read() used in pipes.\n");
    sleep(SLEEP_TIME);
    printf("P : Woke up\n");

    char buf_write[SIZE_OF_BUF_WRITE];
    int bytes_written_to_buf = snprintf(buf_write, sizeof(buf_write), "123456789ABCDEF");
    printf("P : Sending data to the writing end of the pipe\n");
    if (write(fd_pipe[WRITING_END], buf_write, bytes_written_to_buf) == -1)
    {
        perror("P : Error writing buf to pipe's writing end\n");
        exit(EXIT_FAILURE);
    }
    if (close(fd_pipe[WRITING_END]) == -1)
    {
        perror("P : Error closing the writing end of the pipe on my side\n");
    }
    int status;
    cpid = wait(&status);
    switch (cpid)
    {
    case -1:
        perror("P : Error waiting for the child\n");
        exit(EXIT_FAILURE);
    default:
        printf("P : wait() in parent done for %d with a status of %d\n", cpid, status);
        break;
    }
    printf("P : Exiting...\n");
    exit(EXIT_SUCCESS);
}