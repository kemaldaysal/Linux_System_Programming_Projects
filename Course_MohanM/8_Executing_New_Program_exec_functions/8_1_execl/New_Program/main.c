#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    printf("-New Program (%d) : I am a new program called by execl().\n", getpid());
    // Notice that it has the same pid with the main (previous) program

    printf("Arguments provided by execl are:\n");
    for (int i = 0; i < argc; i++)
    {
        printf("argv[%d] : %s\n", i, argv[i]);
    }
    return 0;
}