#include <unistd.h> // lseek, write
#include <fcntl.h>  // open,
#include <stdio.h>  // snprintf
#include <string.h> // strerror
#include <errno.h>  // errno
#include <stdint.h>

const char *FILE_NAME = {"author.txt"};
// #define FILE_MODES (O_RDWR | O_APPEND)
const int FILE_MODES = (O_RDWR);

enum FILE_DESCRIPTORS
{
    FD_STDIN = 0,
    FD_STDOUT = 1,
    FD_STDERR = 2
};

enum SIZE
{
    SIZE_BUF_LOG = 250,
    SIZE_BUF_ERR = 250,
    SIZE_BUF_READ = 250
};

typedef enum
{
    SUCCESS = 0,
    ERR_GENERAL_ERROR = -1,
    ERR_FILE_OPEN = -2,
    ERR_FILE_READ = -3,
    ERR_FILE_WRITE = -4,
    ERR_FILE_CLOSE = -5,
    ERR_ENCODING = -6
} EXIT_TYPES;

const uint8_t STRINGS_TO_READ_AT_ONCE = 1;
const char *STRING_TARGET = "Rohan";
const char *LETTER_CORRECT = "M";

EXIT_TYPES log_error(const char *msg_prefix);
EXIT_TYPES close_file_safer(int fd);

int main()
{
    int fd = open(FILE_NAME, FILE_MODES);
    if (fd == ERR_GENERAL_ERROR)
    {
        log_error("Error opening the file");
        return ERR_FILE_OPEN;
    }

    // READ (not necessary??)

    // FIND 'R' word, or target word = Rohan (if you can)
    // find it's index and replace it by writing replaced_word = 'M'

    char buf_read_bytes[SIZE_BUF_READ] = {'\0'};
    uint8_t total_bytes_read = 0;
    bool letter_replaced_successfully = 0;

    for (;;)
    {
        ssize_t bytes_read = read(fd, buf_read_bytes + total_bytes_read, STRINGS_TO_READ_AT_ONCE);
        static uint8_t target_iterator = 0;

        if (bytes_read == ERR_GENERAL_ERROR)
        {
            log_error("Reading error");
            return ERR_FILE_READ;
        }
        else if (bytes_read == 0)
        {
            log_error("EOF reached");
            break;
        }

        if (buf_read_bytes[total_bytes_read] == STRING_TARGET[target_iterator])
        {
            target_iterator++;

            if (target_iterator == strlen(STRING_TARGET))
            {
                lseek(fd, -(strlen(STRING_TARGET)), SEEK_CUR);
                ssize_t bytes_written_to_fd = write(fd, LETTER_CORRECT, strlen(LETTER_CORRECT));
                if (bytes_written_to_fd == ERR_GENERAL_ERROR)
                {
                    log_error("Error writing to fd");
                    return ERR_FILE_WRITE;
                }
                letter_replaced_successfully = 1;
                break;
            }
        }
        total_bytes_read += bytes_read;
    }

    if (letter_replaced_successfully == 1)
    {
        log_error("Found and replaced the letter successfully.");
    }
    else 
    {
        log_error("Couldn't find the target string");
    }

    if (close_file_safer(fd) != SUCCESS)
    {
        return ERR_FILE_CLOSE;
    }

    return SUCCESS;
}

EXIT_TYPES log_error(const char *msg_prefix)
{
    char buf[SIZE_BUF_ERR] = {0};
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
    else
    {
        return ERR_ENCODING;
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