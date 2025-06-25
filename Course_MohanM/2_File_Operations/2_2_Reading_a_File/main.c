/* ---------------- Notes ------------------ */

/*

A successfull call to read() returns the number of bytes actually read,
returns 0 if end-of-file is encountered
returns -1 on error

read() is used to read from a regular file, a pipe socket and FIFO.

*/

/* ---------------- Libraries ------------------ */

#include <fcntl.h>  // open()
#include <unistd.h> // read(), close()
#include <stdio.h>  // printf(), perror()
#include <string.h> // memset()
#include <errno.h>  // errno
#include <unistd.h> // write()

#include <stdint.h>

/* ---------------- Enumerations, Defines and Constants ------------------ */

#define INPUT_FILE_NAME "input.txt"
#define FILE_MODES (O_RDONLY)

enum BUFFER_SIZES
{
    SIZE_BUF_STORAGE = 30,
    SIZE_BUF_LOG = 100,
    SIZE_BUF_ERROR_LOG = 256
};

static const size_t BYTES_TO_READ_AT_ONCE = 10;

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
    ERR_WRITE_ENCODING = -4,
    ERR_FILE_READ = -5

} EXIT_TYPES;

/* ---------------- Function Prototypes ------------------ */

void log_error(const char *msg_prefix);
EXIT_TYPES close_file_safer(int fd);

/* ---------------- Main Function ------------------ */

int main()
{
    int fd = open(INPUT_FILE_NAME, FILE_MODES);
    if (fd == ERR_GENERAL_ERROR)
    {
        log_error("Error opening the file");
        return ERR_FILE_OPEN;
    }

    ssize_t bytes_read = -1;
    uint8_t total_bytes_read = 0;
    // size_t read_step = 0;
    char buf_storage[SIZE_BUF_STORAGE] = {0};

    for (;;)
    {
        // memset(buf_storage, 0, sizeof(buf_storage)); // clear stale data
        bytes_read = read(fd, buf_storage + total_bytes_read, BYTES_TO_READ_AT_ONCE);

        if (bytes_read == ERR_GENERAL_ERROR)
        {
            log_error("Error reading the file");
            close_file_safer(fd);
            return ERR_FILE_READ;
        }

        else if (bytes_read == 0) // EOF
        {
            log_error("EOF reached");
            break;
        }

        total_bytes_read += bytes_read;

        // char buf_log[SIZE_BUF_LOG] = {0};
        // int msglen = snprintf(buf_log, sizeof(buf_log), "call %zu - read %zd bytes from fd %d:\n<%.*s>\n\n", ++read_step, bytes_read, fd, (int)bytes_read, buf_storage);
        // if (msglen > 0)
        // {
        //     if (msglen > SIZE_BUF_LOG)
        //     {
        //         msglen = SIZE_BUF_LOG - 1;
        //     }
        //     write(FD_STDOUT, buf_log, msglen);
        // }
    }

    char buf_log[SIZE_BUF_LOG] = {0};
    int msglen = snprintf(buf_log, sizeof(buf_log), "Read %d bytes from fd %d:\n<%.*s>\n\n", total_bytes_read, fd, (int)total_bytes_read, buf_storage);
    if (msglen > 0)
    {
        if (msglen > SIZE_BUF_LOG)
        {
            msglen = SIZE_BUF_LOG - 1;
        }
        write(FD_STDOUT, buf_log, msglen);
    }

    close_file_safer(fd);

    return SUCCESS;
}

/* ---------------- Function Implementations ------------------ */

EXIT_TYPES close_file_safer(int fd)
{
    if (close(fd) != SUCCESS)
    {
        log_error("Error closing file");
        return ERR_FILE_CLOSE;
    }
    return SUCCESS;
}

void log_error(const char *msg_prefix)
{
    char buf[SIZE_BUF_ERROR_LOG] = {0};
    int len_written_to_buf = snprintf(buf, sizeof(buf), "%s: %s\n", msg_prefix, strerror(errno));

    if (len_written_to_buf > 0)
    {
        // Cap length to buffer size to avoid overflow on write()
        if (len_written_to_buf > SIZE_BUF_ERROR_LOG)
        {
            len_written_to_buf = SIZE_BUF_ERROR_LOG - 1;
        }

        (void)write(FD_STDERR, buf, len_written_to_buf);
    }
}