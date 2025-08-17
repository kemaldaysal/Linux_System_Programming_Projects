#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

extern char **environ;

int main(int argc, char *argv[])
{
    printf("-P2 (%d): Executing...\n", getpid());
    for (int i=0; i<argc; i++)
        printf("argv[%d] : %s\n", i, argv[i]);

    for (char **ep = environ; *ep != NULL; ep++)
        printf("environ: %s\n", *ep);

    exit(0);
}