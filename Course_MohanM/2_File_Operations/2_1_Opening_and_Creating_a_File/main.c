/* ---------------- Libraries ------------------ */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h> // <-- Required for symbolic permission macros

#include <unistd.h> // for write()
#include <string.h>

/* ---------------- Enumerations, Defines and Constants ------------------ */

#define FILE_NAME "startup.txt"
#define FILE_MODES (O_RDONLY | O_CREAT)
#define FILE_PERMISSIONS (S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH)

enum BUFFER_SIZES
{
    SIZE_BUF = 100,
    SIZE_BUF_ERROR = 256
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
    ERR_FILE_OPEN = -10,
    ERR_FILE_CLOSE = -3,
    ERR_WRITE_ENCODING = -4

} EXIT_TYPES;

/* ---------------- Function Prototypes ------------------ */

void log_error(const char *msg_prefix);
EXIT_TYPES close_file_safer(int fd);

/* ---------------- Main Function ------------------ */

int main()
{
    // int fd = open(FILE_NAME, O_RDONLY); // only read if it exists
    // int fd = open(FILE_NAME, O_RDONLY | O_CREAT, 0664); // create it if it doesn't exist // 0774: (rw-r--r--), must be used with O_CREAT
    // int fd = open(FILE_NAME, O_RDONLY | O_CREAT | O_EXCL, FILE_PERMISSIONS); // if it's created once, don't create it again.
    int fd = open(FILE_NAME, FILE_MODES, FILE_PERMISSIONS); // if it's created once, don't create it again.
    if (fd == ERR_GENERAL_ERROR)
    {
        log_error("Error opening the file");
        return ERR_FILE_OPEN;
    }

    char buf[SIZE_BUF] = {'\0'};
    int msglen = snprintf(buf, sizeof(buf), "open() system call succeeded, file descriptor = %d\n", fd);
    if (msglen > 0)
    {
        if (msglen > SIZE_BUF)
        {
            msglen = SIZE_BUF - 1;
        }
        write(FD_STDOUT, buf, msglen);
    }
    else
    {
        log_error("Encoding error");
        return ERR_WRITE_ENCODING;
    }

    if (close_file_safer(fd) != SUCCESS)
    {
        return ERR_FILE_CLOSE;
    }
    
    return SUCCESS;
}

/* ---------------- Function Implementations ------------------ */

void log_error(const char *msg_prefix)
{
    char buf[SIZE_BUF_ERROR] = {0};
    int len_written_to_buf = snprintf(buf, sizeof(buf), "%s: %s\n", msg_prefix, strerror(errno));

    if (len_written_to_buf > 0)
    {
        if (len_written_to_buf > SIZE_BUF_ERROR)
        {
            len_written_to_buf = SIZE_BUF_ERROR - 1;
        }
        (void)write(FD_STDERR, buf, len_written_to_buf); // do nothing if it fails
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