/*  NOTES
    - read() on the read end (fd_pipe[READING_END]) will only return 0 (EOF) when all "write" descriptors referring to the pipe are closed.
    Otherwise, read() will be waiting for more data to arrive on writing end of the pipe, which is still open.
    - So outside of PIPEs, the basic return of read() was 0 when the read file is reached EOF,
    But with PIPEs, in order to make it return 0, we need to close the writing pipe after we're done, to inform that we won't send further data.
    - If we don't close it, it'll keep waiting for more data instead of just returning 0, since the write end is still open. That's why our while() loop didn't end at first try.
*/

/* ---- Headers ---- */

#include <unistd.h> // for pipe
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* ---- Enumerations and Defines ---- */

#define SIZE_OF_WRITE_BUF 50
#define SIZE_OF_READ_BUF 50 // ensure it's at least BYTES_TO_READ_AT_ONCE + 1
#define BYTES_TO_READ_AT_ONCE 6

enum PIPE_ENDS
{
    READING_END = 0,
    WRITING_END = 1
};

/* ---- Main Function ---- */

int main()
{
    // ----------- Creating the pipe --------------

    int fd_pipe[2];
    if (pipe(fd_pipe) == -1)
    {
        perror("Error opening the pipe\n"); // Error in pipe
        exit(EXIT_FAILURE);
    }

    // ----------- Writing --------------

    char buf_write[SIZE_OF_WRITE_BUF];
    snprintf(buf_write, sizeof(buf_write), "PIPE data flow demo\n");

    if (write(fd_pipe[WRITING_END], buf_write, strlen(buf_write)) == -1)
    {
        perror("Error writing to pipe\n");
        exit(EXIT_FAILURE);
    }

    if (close(fd_pipe[WRITING_END]) == -1) // Important: close the write end after writing
    {
        perror("Error closing writing end of the pipe\n");
        exit(EXIT_FAILURE);
    }

    // ----------- Reading --------------

    char buf_read[SIZE_OF_READ_BUF];
    int bytes_read = 0;

    while ((bytes_read = read(fd_pipe[READING_END], buf_read, BYTES_TO_READ_AT_ONCE)) > 0) // end until the end of the file
    {
        buf_read[bytes_read] = '\0'; // null-terminate for safe printing
        printf("%d bytes read, which are: %s\n", bytes_read, buf_read);
    }

    printf("Exiting...\n");
    return 0;
}