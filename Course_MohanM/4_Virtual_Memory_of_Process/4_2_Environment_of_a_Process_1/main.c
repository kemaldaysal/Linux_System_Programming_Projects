/* Simple program to list the environment of a process */
#include <stdio.h>

extern char **environ; // holds all the list of environment that is used in the process

int main(int argc, char *argv[])
{
    for (char **ep = environ; *ep != NULL; ep++)
    {
        printf("%s\n", *ep);
    }
}