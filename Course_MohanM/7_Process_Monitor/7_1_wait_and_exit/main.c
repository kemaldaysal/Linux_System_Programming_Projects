#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

/*  Explanation of returned "status"
Process termination status (int) 16 bit number
1. status value if child process has normal exit/termination:
    15......8       |      7......0
     00000011 (3)   |      00000000 = 2^9 + 2^8 = 768

2. status if child process is killed by signal (to shell: kill -9 "child PID" )
    15......8       |      7  ......0
     unused         |      1  XXXXXXX
                           | -> core dump flag
                              ||||||| (signal used when killing (kill -"8", -"9" etc))
*/

enum MACROS
{
    T_SLEEP = 10

};

int main()
{
    int status = 0;

    pid_t cpid = fork();
    switch (cpid)
    {
    case -1:
        exit(-1);

    case 0:
        printf("-C (PID: %d): Started executing, now sleeping for %d seconds\n", getpid(), T_SLEEP);
        sleep(T_SLEEP);
        printf("-C: Exiting after waking up\n");
        exit(3);

    default:
        printf("--P: (PID: %d): Started executing, now i'll wait() for my childs to finish executing\n", getpid());
        // cpid = wait(NULL); // do the same without returning status info
        cpid = wait(&status);
        if (cpid == -1)
        {
            perror("--P: Wait error for child\n");
            exit(-1);
        }

        printf("--P: Wait is done for a child (ID: %d), returned status is: %d\n", cpid, status);

        if (status != 0 && status < 256)
        {
            printf("--P: Child (%d) is killed by a signal!!",cpid);
        }

        printf("--P: Parent exiting\n");
    }
    return 0;
}