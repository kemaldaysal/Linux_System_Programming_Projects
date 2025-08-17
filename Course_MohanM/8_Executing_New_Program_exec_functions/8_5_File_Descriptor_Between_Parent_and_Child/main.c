/*
NOTES
TO DO
    - Open a file from parent, write something to it, then fork and create a child,
    then write to the same fd on the child to see whether it remains on the same cursor or not.
*/

/* ---------------- Libraries ----------------*/

#include <fcntl.h>
#include <sys/stat.h>

#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <errno.h>

#include <sys/wait.h>

/* ---------------- Macros and Enumerations ----------------*/

#define OUTPUT_FILE_NAME "test.txt"
#define FILE_MODES (O_RDWR | O_TRUNC | O_CREAT)
#define FILE_PERMISSIONS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)

enum BUFFER_SIZES
{
    SIZE_TX_BUF = 30,
    SIZE_BUF_ERROR_LOG = 256,
};

enum TIMERS
{
    T_C_SLEEP = 2
};

enum FILE_DESCRIPTORS
{
    FD_STDIN = 0,
    FD_STDOUT = 1,
    FD_STDERR = 2
};

typedef enum
{
    SUCCESS = 0,
    ERR_GENERAL_ERROR = -1,
    ERR_FILE_OPEN = -2,
    ERR_FILE_CLOSE = -3,
    ERR_FILE_WRITE = -4

} EXIT_TYPES;

/* ---------------- Function Prototypes ----------------*/

void log_error(const char *msg_prefix);
EXIT_TYPES close_file_safer(int fd);
EXIT_TYPES write_to_a_file_safer(int fd, const char *txbuf);

/* ---------------- Main Function ----------------*/

int main()
{

    int fd = open(OUTPUT_FILE_NAME, FILE_MODES, FILE_PERMISSIONS);
    if (fd == ERR_GENERAL_ERROR)
    {
        log_error("Error opening the file");
        return ERR_FILE_OPEN;
    }

    printf("-P (%d) : Opened text file (%s) with file descriptor of (%d).\n", getpid(), OUTPUT_FILE_NAME, fd);

    if (write_to_a_file_safer(fd, "Parent is writing...\n") != SUCCESS)
    {
        exit(ERR_FILE_WRITE);
    }

    int status = 0;

    pid_t cpid = fork();
    switch (cpid)
    {
    case -1:
        log_error("Fork failed!!");
        exit(ERR_GENERAL_ERROR);

    case 0:
        printf("--C (%d) : Started executing... I'll sleep for %d seconds for testing closed-file on parent scenario.\n", getpid(), T_C_SLEEP);
        sleep(T_C_SLEEP);

        printf("--C (%d) : Woke up, fd on parent is closed while i'm sleeping but i still have my own copy. I'll write to the file (fd : %d).\n", getpid(), fd);
        if (write_to_a_file_safer(fd, "Child is writing...\n") != SUCCESS)
        {
            exit(ERR_FILE_WRITE);
        }

        close_file_safer(fd); // Child must close it's own copy of fd when it's finished with file (Although OS closes it after child finished executing, but it's bad practice to leave it to OS.)
        
        printf("--C (%d) : Child is exiting...\n", getpid());        
        exit(SUCCESS);        // On exits, kernel cleans up all the fds of the process. So it closes the file if it's forgotten but it remains open unnecessarily, hence wasting resources.

    default:
        close_file_safer(fd); // Parent closes it's own copy of fd since it's finished, and after successful fork, fd is already copied to child.

        printf("-P (%d) : Fork success, i'll wait for my child to finish.\n", getpid());
        pid_t pid_c_waited = waitpid(cpid, &status, 0);
        switch (pid_c_waited)
        {
        case -1:
            printf("-P (%d) : Wait failed for child (%d)\n", getpid(), cpid);
            exit(-1);

        default:
            printf("-P (%d) : Wait is done for child (%d), returned status is %d.\n", getpid(), pid_c_waited, status);
        }

        printf("-P (%d) : Parent is exiting...\n", getpid());

    }
    return SUCCESS;
}

/* ---------------- Function Implementations ----------------*/

EXIT_TYPES write_to_a_file_safer(int fd, const char *txbuf)
{
    ssize_t bytes_written_to_file = write(fd, txbuf, strlen(txbuf));
    if (bytes_written_to_file == ERR_GENERAL_ERROR)
    {
        log_error("Error writing to file");
        close_file_safer(fd);
        return ERR_FILE_WRITE;
    }
    return SUCCESS;
}

void log_error(const char *msg_prefix)
{
    char buf[SIZE_BUF_ERROR_LOG] = {'\0'};
    int len_written_to_buf = snprintf(buf, sizeof(buf), "%s : %s\n", msg_prefix, strerror(errno));
    if (len_written_to_buf > 0)
    {
        if (len_written_to_buf > SIZE_BUF_ERROR_LOG)
        {
            len_written_to_buf = SIZE_BUF_ERROR_LOG - 1;
        }
        (void)write(FD_STDERR, buf, len_written_to_buf);
    }
}

EXIT_TYPES close_file_safer(int fd)
{
    if (close(fd) != SUCCESS)
    {
        log_error("Error closing file");
        return ERR_FILE_CLOSE;
    }
    return SUCCESS;
}