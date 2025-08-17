#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    printf("-P2 (%d) : I am the new program called by execv()\n", getpid());
    
    for (int i = 0; i < argc; i++)
        printf("argv[%d] : %s\n", i, argv[i]);

    return 0;
}