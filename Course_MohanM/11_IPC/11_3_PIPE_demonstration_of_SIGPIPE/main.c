/*  NOTES
 - Before sending 2nd part of the data, we'll intentionally get "broken pipe" error there, caused by SIGPIPE exception.
 - That's because child has closed the reading end of the pipe before parent sent the 2nd part of the data.
 - It'll be waste to send the data, since child can't listen it's parent anymore.
 - So by default, SIGPIPE makes the process exit, which is the process who receives SIGPIPE.
 - We can do signal handling for SIGPIPE to not to immediately terminate the process.
*/

/* ---- Headers ---- */

#define _POSIX_C_SOURCE 200809L // used to ignore SIG_PIPE
#include <signal.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <wait.h>
#include <errno.h>

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

#define BYTES_TO_READ_AT_ONCE 4
#define SLEEP_TIME 2

/* ---- Main Function ---- */

int main()
{
    printf("P (%d) : Started executing...\n", getpid());

    signal(SIGPIPE, SIG_IGN); // To avoid process termination by SIGPIPE 
    // sigaction must be used since signal is obsolete, only for demonstration!!

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

                // Now cut the connection and close the reading pipe connected to child
                printf("-C : For some reason, i have to close my reading end of the pipe\n");
                if (close(fd_pipe[READING_END]) == -1)
                {
                    printf("-C : Error closing the reading end of the pipe\n");
                    exit(EXIT_FAILURE);
                }
                break;
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

    char buf_write[SIZE_OF_BUF_WRITE];
    int bytes_written_to_buf = snprintf(buf_write, sizeof(buf_write), "1234");
    printf("P : Sending 1st part of the data to the writing end of the pipe\n");
    if (write(fd_pipe[WRITING_END], buf_write, bytes_written_to_buf) == -1)
    {
        if (errno == EPIPE)
        {
            printf("P : Reader closed the pipe (write returned EPIPE)\n");
            printf("P : Since SIG_PIPE signal is ignored, the program will continue\n");
        }
        else 
        {
            perror("P : Error writing buf to pipe's writing end\n");
            // exit(EXIT_FAILURE);
        }
    }

    printf("P : I'll sleep before sending the second part of the data.\n");
    sleep(SLEEP_TIME); // Child closes the reading end of the pipe while parent sleeps

    printf("P : Woke up.\n");
    bytes_written_to_buf = snprintf(buf_write, sizeof(buf_write), "5678");
    printf("P : Sending 2nd part of the data to the writing end of the pipe\n");
    if (write(fd_pipe[WRITING_END], buf_write, bytes_written_to_buf) == -1) // It'll cause SIGPIPE signal sent to parent
    {
        if (errno == EPIPE)
        {
            printf("P : Reader closed the pipe (write returned EPIPE)\n");
            printf("P : Since SIG_PIPE signal is ignored, the program will continue\n");
        }
        else 
        {
            perror("P : Error writing buf to pipe's writing end\n");
            // exit(EXIT_FAILURE);
        }
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