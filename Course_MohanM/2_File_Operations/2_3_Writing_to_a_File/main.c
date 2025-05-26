/* ---------------- Libraries ------------------ */

#include <fcntl.h>  // for open()
#include <unistd.h> // for write()
#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <sys/stat.h> // <-- Required for symbolic permission macros

/* ---------------- Enumerations, Defines and Constants ------------------ */

#define OUTPUT_FILE_NAME "output.txt"
#define FILE_MODES (O_WRONLY | O_APPEND | O_CREAT)
// #define FILE_MODES (O_WRONLY | O_TRUNC | O_CREAT)
#define FILE_PERMISSIONS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)

enum BUFFER_SIZES
{
    SIZE_TX_BUF = 30,
    SIZE_BUF_ERROR_LOG = 256,
    SIZE_BUF_FINAL_LOG = 256
};

typedef enum
{
    FD_STDIN = 0,
    FD_STDOUT = 1,
    FD_STDERR = 2
} FILE_DESCRIPTORS;

typedef enum
{
    SUCCESS = 0,
    ERR_GENERAL_ERROR = -1,
    ERR_FILE_OPEN = -10,
    ERR_FILE_WRITE = -2,
    ERR_FILE_CLOSE = -3,
    ERR_WRITE_ENCODING = -4

} EXIT_TYPES;

/* ---------------- Function Prototypes ------------------ */

EXIT_TYPES write_to_a_file_safer(int fd, const char *buf, size_t len);
void log_error(const char *msg_prefix);
EXIT_TYPES close_file_safer(int fd);

/* ---------------- Main Function ------------------ */

int main()
{
    int fd = open(OUTPUT_FILE_NAME, FILE_MODES, FILE_PERMISSIONS);
    if (fd == ERR_GENERAL_ERROR)
    {
        log_error("Error opening the file");
        return ERR_FILE_OPEN;
    }

    char tx_buf[SIZE_TX_BUF] = {'\0'};
    snprintf(tx_buf, sizeof(tx_buf), "Hello World\n");

    if (write_to_a_file_safer(fd, tx_buf, strlen(tx_buf)) != SUCCESS)
    {
        return ERR_FILE_WRITE;
    }

    if (close_file_safer(fd) != SUCCESS)
    {
        return ERR_FILE_CLOSE;
    }

    return 0;
}

/* ---------------- Function Implementations ------------------ */

EXIT_TYPES write_to_a_file_safer(int fd, const char *buf, size_t len)
{
    size_t total_written = 0;

    while (total_written < len)
    {
        ssize_t bytes_written = write(fd, buf + total_written, len - total_written);
        if (bytes_written == ERR_GENERAL_ERROR)
        {
            log_error("Error writing to file");
            close_file_safer(fd);
            return ERR_FILE_WRITE;
        }
        total_written += bytes_written;
    }

    char buf_log[SIZE_BUF_FINAL_LOG] = {0};
    int len_written_to_buf_log = snprintf(buf_log, sizeof(buf_log), "Successfully wrote %zu bytes written to file descriptor %d\n", total_written, fd);
    if (len_written_to_buf_log > 0)
    {
        if (len_written_to_buf_log > SIZE_BUF_FINAL_LOG)
        {
            len_written_to_buf_log = SIZE_BUF_FINAL_LOG - 1;
        }
        write(FD_STDOUT, buf_log, len_written_to_buf_log);
    }
    else
    {
        log_error("Encoding error");
        return ERR_WRITE_ENCODING;
    }

    return SUCCESS;
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

void log_error(const char *msg_prefix)
{
    char buf[SIZE_BUF_ERROR_LOG] = {0};
    int len_written_to_buf = snprintf(buf, sizeof(buf), "%s: %s\n", msg_prefix, strerror(errno));
    if (len_written_to_buf > 0)
    {
        if (len_written_to_buf > SIZE_BUF_ERROR_LOG)
        {
            len_written_to_buf = SIZE_BUF_ERROR_LOG - 1;
        }
        (void)write(FD_STDERR, buf, len_written_to_buf);
    }
}