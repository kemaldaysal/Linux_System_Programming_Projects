/* Demo program to show the command line arguments of process */
/* ** Command line arguments are stored as part of stack of the main program.
**    When this program is executed, a new process is forked/created, and this 
     command line argument forms the part of the stack frame of the main function. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char *argv[])
{   
    printf("Demonstrate the command line arguments\n");

    if (strstr(argv[1], "add"))
    {
        printf("Addition result = %d\n", (atoi(argv[2]) + atoi(argv[3])));
    }
    else if (strstr(argv[1], "sub"))
    {
        printf("Subtraction result = %d\n", (atoi(argv[2]) - atoi(argv[3])));
    }
    else 
    {
        printf("Wrong input\n");
    }
    return 0;
}