#include <stdio.h>
#include <unistd.h>

int main()
{
    printf("-Main Process (%d): Started executing\n", getpid());
    execl("../New_Program/main", "arg1", "arg2", "arg3", NULL);

    printf("This line won't be printed since all the virtual memory of new program is copied into this main process");

    return 0;
}