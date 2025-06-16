/* create a new environment variable and set it's value */

#include <stdio.h>
#include <stdlib.h>

/* the command line arguments variable is visible only in main(), 
whereas the environment variables are visible all over the process */

void call_func_1(void);

int main()
{
    putenv("PARAM1=1024");

    printf("PATH : (%s)\n", getenv("PATH"));
    printf("HOME : (%s)\n", getenv("HOME"));
    printf("PARAM1 (added by user) : (%s)\n", getenv("PARAM1"));

    call_func_1();

    return 0;
}

void call_func_1(void)
{
    printf("Inside function call_func_1: PARAM1 (added by user): %s\n", getenv("PARAM1"));
}