#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

enum MACROS
{
    T_SLP_C1 = 2,
    T_SLP_C2 = 3
};

int main()
{
    int status = 0;

    pid_t cpid_1 = fork();

    switch (cpid_1)
    {
    case -1:
        perror("Fork error on child 1!!\n");
        exit(-1);

    case 0:
        printf("-C1 (PID: %d): Started executing, now sleeping for %d seconds\n", getpid(), T_SLP_C1);
        sleep(T_SLP_C1);
        printf("-C1: Exiting after waking up...\n");
        exit(0);

    default:

        pid_t cpid_2 = fork();
        switch (cpid_2)
        {
        case -1:
            perror("Fork error on child 2!!\n");
            exit(-1);
        case 0:
            printf("-C2 (PID: %d): started executing, now sleeping for %d seconds\n", getpid(), T_SLP_C2);
            sleep(T_SLP_C2);
            printf("-C2: Exiting after waking up...\n");
            exit(1);
        }

        printf("--P: (PID: %d): Started executing, now i'll wait.\n", getpid());
        pid_t cpid = wait(&status);
        if (cpid == -1)
        {
            perror("--P: Wait error for 1st child\n");
            exit(-1);
        }

        printf("--P: Wait is done for a child (ID: %d), returned status is: %d\n", cpid, status);

        if (status != 0 && status < 256)
        {
            printf("--P: Child (%d) is killed by a signal!!",cpid);
        }

        cpid = wait(&status);
        if (cpid == -1)
        {
            perror("--P: Wait error for 2nd child\n");
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