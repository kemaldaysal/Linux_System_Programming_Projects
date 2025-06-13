/* Demo program to show the command line arguments of process */

#include <stdio.h>
#include <string.h>

/* argc and argv are command line arguments and are stored in stack frame of main() */
/* argv is array of pointers to string */

int main(int argc, char *argv[])
{

    printf("Demonstrate the command line arguments\n");
    printf("The value of argc is: %d\n", argc);

    unsigned int count = 0;

    while (count < argc)
    {
        printf("%dth string is: %s\n", count, argv[count]);
        count++;
    }

    return 0;
}