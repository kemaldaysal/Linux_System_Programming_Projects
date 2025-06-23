/* Use of WNOHANG, which is a non-blocking call to waitpid()*/
/*
    Is used when:
    * You don’t want to block the parent process.
    * You want to check if a child has exited, and if not, continue doing other work.


*/

#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

enum MACROS
{
    T_SLP_C1 = 6,
    T_SLP_C2 = 1
};

int main()
{
    int status = 0;
    pid_t cpid_1 = fork();
    pid_t ret_pid;

    switch (cpid_1)
    {
    case -1:
        perror("Fork failed!!\n");
        exit(-1);
    case 0:
        printf("-C1 (%d): Started executing, now i'll sleep for %d seconds...\n", getpid(), T_SLP_C1);
        sleep(T_SLP_C1);
        printf("-C1: Woke up, now i'll exit.\n");
        exit(0);
    default:

        pid_t cpid_2 = fork();
        switch (cpid_2)
        {
        case -1:
            perror("Fork failed!!\n");
        case 0:
            printf("-C2 (%d): Started executing, now i'll sleep for %d seconds...\n", getpid(), T_SLP_C2);
            sleep(T_SLP_C2);
            printf("C2: Woke up, now i'll exit.\n");
            exit(1);
        }

        printf("--P (%d): Started executing, now i'll wait for my CHILD 1 (specific)...\n", getpid());

        ret_pid = waitpid(cpid_1, &status, WNOHANG);
        // ret_pid = waitpid(-1, &status, WNOHANG); // -1 matches ANY process like wait()
        // ret_pid = waitpid(80, &status, WNOHANG); // Error scenario: Trying it with unknown, unexpected PID

        switch (ret_pid)
        {
        case -1:
            perror("Wait failed for child 1\n");
            exit(-1);
        case 0:
            printf("Child %d hasn't exited yet.\n", cpid_1);
            break;

        default:
            printf("--P: Wait is done for SPECIFIC child (ID: %d), returned status is: %d\n", cpid_1, status);
        }

        printf("--P (%d): Now i'll wait for my CHILD 2 (specific)...\n", getpid());
        ret_pid = waitpid(cpid_2, &status, WNOHANG);

        switch (ret_pid)
        {
        case -1:
            perror("Wait failed for child 2\n");
            exit(-1);
        case 0:
            printf("Child %d hasn't exited yet.\n", cpid_2);
            break;

        default:
            printf("--P: Wait is done for SPECIFIC child (ID: %d), returned status is: %d\n", cpid_2, status);
        }

        printf("--P: Parent exiting\n");
    }

    return 0;
}