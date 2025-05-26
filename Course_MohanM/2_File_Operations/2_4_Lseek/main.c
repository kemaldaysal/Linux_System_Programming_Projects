/* ---------------- Libraries ------------------ */
#include <unistd.h> // lseek, write
#include <fcntl.h>  // open,
#include <stdio.h>  // snprintf
#include <string.h> // strerror
#include <errno.h>  // errno

/* ---------------- Enumerations, Defines, Constants ------------------ */

#define FILE_NAME ("input.txt")
#define FILE_MODES (O_RDWR | O_TRUNC)
const size_t bytes_to_read_at_once = 10;

enum BUFFER_SIZES
{
    SIZE_BUF_READ = 100,
    SIZE_BUF_TX = 256,
    SIZE_BUF_LOG = 256
};

typedef enum
{
    SUCCESS = 0,
    ERR_GENERAL = -1,
    ERR_OPEN_ERROR = -2,
    ERR_WRITE = -3,
    ERR_ENCODING = -4,
    ERR_FILE_CLOSE = -5

} EXIT_TYPES;

enum FILE_DESCRIPTORS
{
    FD_STDIN = 0,
    FD_STDOUT = 1,
    FD_STDERR = 2
};

/* ---------------- Function Prototypes ------------------ */

void log_seek_position(int seek_pos);
EXIT_TYPES read_and_log_safe(int fd, size_t bytes_to_read_at_once);
EXIT_TYPES write_to_a_file_and_log_safe(int fd, const char *text_to_write);
void log_error(const char *msg_prefix);
EXIT_TYPES close_file_safer(int fd);

/* ---------------- Main Function ------------------ */

int main()
{
    int fd = open(FILE_NAME, FILE_MODES);
    if (fd == ERR_GENERAL)
    {
        log_error("Error opening file");
        return ERR_OPEN_ERROR;
    }

    int seek_pos = 0;

    seek_pos = lseek(fd, 0, SEEK_SET);
    log_seek_position(seek_pos); // 0

    write_to_a_file_and_log_safe(fd, "1234567891011121314"); // start from the beginning

    seek_pos = lseek(fd, 0, SEEK_CUR); // just to track current position after write
    log_seek_position(seek_pos);       // 0 + 6 = shifted to 6 after writing 123456

    seek_pos = lseek(fd, 2, SEEK_SET);
    log_seek_position(seek_pos); // 2

    write_to_a_file_and_log_safe(fd, "00"); // // Elements 34 are gone, REPLACED by 00!!
    // Elements doesn't shift automatically. But seek_pos shifted automatically by 2 !

    seek_pos = lseek(fd, 6, SEEK_CUR);
    log_seek_position(seek_pos); // 2 + 2 (written 00) + 6 = 10

    read_and_log_safe(fd, bytes_to_read_at_once);

    seek_pos = lseek(fd, 0, SEEK_END); // start writing from the end
    log_seek_position(seek_pos);       // 10 + 9 = 19

    write_to_a_file_and_log_safe(fd, "tabbed text");

    seek_pos = lseek(fd, 0, SEEK_CUR); // just to track current
    log_seek_position(seek_pos);       // 19 + 11 = 30

    seek_pos = lseek(fd, 0, SEEK_SET); // go to start
    log_seek_position(seek_pos);       // 0

    read_and_log_safe(fd, 19);

    if (close_file_safer(fd) != SUCCESS)
    {
        return ERR_FILE_CLOSE;
    }
    return SUCCESS;
}

void log_seek_position(int seek_pos)
{
    char buf[SIZE_BUF_LOG] = {'\0'};

    int len_written_to_buf = snprintf(buf, sizeof(buf), "Offset position: (%d)\n", seek_pos);
    if (len_written_to_buf > 0)
    {
        if (len_written_to_buf > SIZE_BUF_LOG)
        {
            len_written_to_buf = SIZE_BUF_LOG - 1;
        }
        write(FD_STDOUT, buf, len_written_to_buf);
    }
}

EXIT_TYPES read_and_log_safe(int fd, size_t bytes_to_read_at_once)
{
    char buf_read[SIZE_BUF_READ] = {'\0'};
    ssize_t len_bytes_read = read(fd, buf_read, bytes_to_read_at_once);

    char buf_log[SIZE_BUF_LOG] = {'\0'};
    int len_written_to_buf = snprintf(buf_log, sizeof(buf_log), "%zd bytes read from fd(%d) after lseek, which are:\n%.*s\n", len_bytes_read, fd, (int)len_bytes_read, buf_read);
    if (len_written_to_buf > 0)
    {
        if (len_written_to_buf > SIZE_BUF_LOG)
        {
            len_written_to_buf = SIZE_BUF_LOG - 1;
        }
        write(FD_STDOUT, buf_log, len_written_to_buf);
        return SUCCESS;
    }
    else
    {
        log_error("Encoding error");
        return ERR_ENCODING;
    }
}

EXIT_TYPES write_to_a_file_and_log_safe(int fd, const char *text_to_write)
{
    char tx_buf[SIZE_BUF_TX];
    int len_bytes_written_to_buf = snprintf(tx_buf, sizeof(tx_buf), "%s", text_to_write);

    if (len_bytes_written_to_buf > 0)
    {
        if (len_bytes_written_to_buf > SIZE_BUF_TX)
        {
            len_bytes_written_to_buf = SIZE_BUF_TX - 1;
        }
        ssize_t len_bytes_written_to_file = write(fd, tx_buf, len_bytes_written_to_buf);

        if (len_bytes_written_to_file <= 0)
        {
            log_error("Error writing or couldn't write anything");
            return ERR_WRITE;
        }

        return SUCCESS;
    }
    else
    {
        log_error("Encoding error");
        return ERR_ENCODING;
    }
}

void log_error(const char *msg_prefix)
{
    char buf[SIZE_BUF_LOG] = {0};
    int len_written_to_buf = snprintf(buf, sizeof(buf), "%s: %s\n", msg_prefix, strerror(errno));

    if (len_written_to_buf > 0)
    {
        if (len_written_to_buf > SIZE_BUF_LOG)
        {
            len_written_to_buf = SIZE_BUF_LOG - 1;
        }
        write(FD_STDERR, buf, len_written_to_buf);
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