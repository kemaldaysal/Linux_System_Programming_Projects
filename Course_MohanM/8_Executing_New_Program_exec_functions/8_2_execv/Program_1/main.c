#include <unistd.h>
#include <stdio.h>

int main()
{
    char *args[] = {"arg1", "arg2", "arg3", NULL};
    execv("../Program_2/main", args);

    printf("This line won't be printed\n");

    return 0;
}