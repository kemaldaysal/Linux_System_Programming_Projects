/*
    NOTES
        To demonstrate more clearly, run command "ps aux | grep Z" on terminal, just after starting and then once more after parent's sleep ends..
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

#include <sys/wait.h>

enum MACROS
{
    T_SLP_P_1 = 10,
    T_SLP_P_2 = 6,
    T_SLP_C = 5
};

int main()
{

    printf("--P (%d): Started executing before fork. Now i'll sleep for %d seconds.\n", getpid(), T_SLP_P_1);

    pid_t id = fork();

    switch (id)
    {
    case -1:
        perror("Fork failed!\n");
        exit(-1);

    case 0:
        printf("-C (%d): Started executing, my parent is %d. now i'll sleep for %d seconds...\n", getpid(), getppid(), T_SLP_C);
        sleep(T_SLP_C);
        printf("-C (%d): Basic check after waking up, my parent is %d\n", getpid(), getppid());
        exit(2);

        // In order to remove the zombie, we need to have a wait in the parent process.

    default:
        sleep(T_SLP_P_1);
        printf("--P (%d): Woke up. I have created child process with ID (%d)\n", getpid(), id);
        printf("(Here, you should run 'ps aux | grep Z' on console)\n");
        waitpid(id, NULL, 0); // This'll remove the zombie state of the child process

        // So once the parent collects the exit status of the child process, the zombie state is removed.

        sleep(T_SLP_P_2);
        printf("--P (%d): Exiting...\n", getpid());
    }

    return 0;
}