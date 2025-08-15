#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

enum MACROS
{
    T_SLP_C = 3
};

int main()
{
    pid_t id = fork();

    switch (id)
    {
    case -1:
        perror("Fork failed!\n");
        exit(-1);

    case 0:
        printf("-C (%d): Started executing, my parent is %d. now i'll sleep for %d seconds...\n", getpid(), getppid(), T_SLP_C);
        sleep(T_SLP_C);
        printf("-C: Woke up, my parent is already gone, i'm adopted by %d. Now i'll exit.\n", getppid());
        exit(2);

    default:
        printf("--P (%d): Started executing.\n", getpid());
        printf("--P (%d): Exiting...\n", getpid());
        // Parent process exits before the child (child is still sleeping in this case).
        // This makes child orphan
        // Child process is adopted by (usually) init (PID: 0) process.
    }

    return 0;
}