/* ---------------- Libraries ------------------ */

#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <errno.h>

/* ---------------- Enumerations, Defines, Constants ------------------ */

const char *msg = "An example to display the output string on screen by Linux system calls\n";

enum BUFFER_SIZES
{
	SIZE_BUF = 100,
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
	ERR_ENCODING = -2
} EXIT_TYPES;

/* ---------------- Main Function ------------------ */

int main()
{
	char buf[SIZE_BUF];
	int len_written_to_buf = snprintf(buf, sizeof(buf), "%s", msg);

	if (len_written_to_buf > 0)
	{
		if (len_written_to_buf > SIZE_BUF)
		{
			len_written_to_buf = SIZE_BUF - 1;
		}

		if (write(FD_STDOUT, buf, len_written_to_buf) == ERR_GENERAL_ERROR)
		{
			return ERR_GENERAL_ERROR;
		}
		return SUCCESS;
	}
	else
	{
		return ERR_ENCODING;
	}
}
