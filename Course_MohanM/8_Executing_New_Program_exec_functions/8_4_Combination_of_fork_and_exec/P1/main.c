#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>

int main()
{

    int status = 0;
    pid_t cpid = fork();
    switch (cpid)
    {
    case -1:

        printf("Fork failed!\n");
        exit(-1); // terminate child

    case 0:
        printf("-C (%d) : Before execl\n", getpid());
        execl("../P2/main", "arg1", "arg2", NULL);
        printf("This line won't be printed\n");
        exit(0);

    default:
        printf("--P (%d) : Executing before wait().\n", getpid());
        cpid = wait(&status);
        printf("--P (%d) : wait() in parent done for %d with a status of %d", getpid(), cpid, status);
    }

    return 0;
}