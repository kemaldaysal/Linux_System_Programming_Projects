#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    printf("-Program 2 (%d) : I am a new process called by execl()\n", getpid());
    printf("-Program 2 (%d) : The passed arguments are:\n", getpid());

    for (int i = 0; i < argc; i++)
        printf("argv[%d] : %s\n", i, argv[i]);

    sleep(3);
    return 5;
}