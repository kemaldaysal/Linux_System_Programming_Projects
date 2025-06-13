// ---------------- Notes and Information
/*
    One of the application of dup is to redirect the output to the file instead of the screen
*/

// ---------------- Libraries and Includes

#include <unistd.h> // for dup()

#include <fcntl.h>    // for file modes
#include <sys/stat.h> // for file permission macros
#include <stdio.h>    // for snprintf
#include <string.h>   // for strerror
#include <errno.h>    // for errno

// ---------------- Defines, Enums, Constants

#define FILE_NAME "newFile.log"
#define FILE_MODES (O_RDWR | O_CREAT)
#define FILE_PERMISSIONS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)

typedef enum
{
    SUCCESS = 0,
    ERR_GENERAL = -1,
    ERR_FILE_OPEN = -2,
    ERR_FILE_CLOSE = -3,
    ERR_ENCODING = -4,
    ERR_F_WRITE = -5
} RETURN_TYPES;

enum SIZE
{
    SIZE_BUF_LOG = 100,
    SIZE_BUF_ERR = 100,
};

enum FILE_DESCRIPTORS
{
    FD_STDIN = 0,
    FD_STDOUT = 1,
    FD_STDERR = 2
};

// ---------------- Function Prototypes

RETURN_TYPES log_error(const char *msg_prefix);
RETURN_TYPES log_fd(const char *msg_prefix, int fd);
RETURN_TYPES close_file_safer(int fd);

// ---------------- Main Function

int main()
{
    int fd = open(FILE_NAME, FILE_MODES, FILE_PERMISSIONS);
    if (fd == ERR_GENERAL)
    {
        log_error("Error opening the file");
        return ERR_FILE_OPEN;
    }

    /* // A more basic version
    close_file_safer(FD_STDOUT); // closed the standard output(screen), fd=1 is free now
    int newfd = dup(fd); // it'll use 1 as new fd, since 1 is the smallest fd available
    */

    // More modern and controllable version that also works in atomic manner
    int newfd = dup2(fd, FD_STDOUT); // close the fd at 2nd parameter and take it's place as new fd, more controllable
    log_fd("newfd", newfd);

    printf("This will be written to file, instead of screen (stdout).\n");

    if (close_file_safer(fd) != SUCCESS)
    {
        return ERR_FILE_CLOSE;
    }
    
    if (close_file_safer(newfd) != SUCCESS)
    {
        return ERR_FILE_CLOSE;
    }

    return SUCCESS;
}

// ---------------- Function Implementations

RETURN_TYPES log_fd(const char *msg_prefix, int fd)
{
    char buf[SIZE_BUF_LOG] = {'\0'};
    int bytes_written_to_buf = snprintf(buf, sizeof(buf), "%s: %d\n", msg_prefix, fd);
    if (bytes_written_to_buf > 0)
    {
        if (bytes_written_to_buf > SIZE_BUF_LOG)
        {
            bytes_written_to_buf = SIZE_BUF_LOG - 1;
        }
        write(FD_STDOUT, buf, bytes_written_to_buf);
        return SUCCESS;
    }

    log_error("Error writing to buf");
    return ERR_ENCODING;
}

RETURN_TYPES log_error(const char *msg_prefix)
{

    char buf[SIZE_BUF_ERR] = {'\0'};
    int bytes_written_to_buf = snprintf(buf, sizeof(buf), "%s: %s\n", msg_prefix, strerror(errno));
    if (bytes_written_to_buf > 0)
    {
        if (bytes_written_to_buf > SIZE_BUF_ERR)
        {
            bytes_written_to_buf = SIZE_BUF_ERR - 1;
        }
        write(FD_STDERR, buf, bytes_written_to_buf);
        return SUCCESS;
    }
    return ERR_ENCODING;
}

RETURN_TYPES close_file_safer(int fd)
{
    if (close(fd) != SUCCESS)
    {
        log_error("Error closing the file");
        return ERR_FILE_CLOSE;
    }
    return SUCCESS;
}