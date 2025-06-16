#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

// ASK FOR DEBUGGING TIPS TO CHATGPT

int main()
{
    printf("Parent Process's ID before fork (PID: %d\n", getpid()); // This won't be executed in child
    pid_t id = fork();

    // ---------------- Only below codes (just after fork) will be executed in child, above won't be executed

    switch (id)
    {
    case -1:
        perror("Fork failed\n");
        exit(-1);

        break;
    case 0:
        printf("In child, after successful fork, %d is returned to child.\n", id);

        printf("In child, process ID (PID): %d\n", getpid());
        printf("In child, Creator/parent's ID (PPID): %d\n", getppid());

        break;
    default:
        printf("In parent, after successful fork, created child's process ID (%d) is returned to parent.\n", id);
    }

    return 0;
}