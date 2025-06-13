/* Process ID and Parent Process ID */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main()
{
    printf("Process ID (PID) is (%d)\n", getpid());
    printf("Process ID Of creator (parent) (PPID) is: (%d)\n", getppid());

    sleep(5);
}