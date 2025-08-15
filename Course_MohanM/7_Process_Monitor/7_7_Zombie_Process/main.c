#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

enum MACROS
{
    T_SLP_P = 10,
    T_SLP_C = 5
};

int main()
{

    printf("--P (%d): Started executing before fork. Now i'll sleep for %d seconds.\n", getpid(), T_SLP_P);

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

        // At this point, since the child exits before the parent, child becomes zombie.
        // Although child exits before parent, there's still an entry for child process in the process table.
        // In action, child process doesn't exist, though all the virtual memory is deallocated and all the resources
        // corresponding to the process is removed. But still the entry of this child process is present in the process table.
        // This is caused by, the parent didn't collect the exit status of the child process. Child has exited while the parent was sleeping.
        // But the parent couldn't catch it.
        // So this is called as a zombie.
        
        // In order to remove the zombie, we need to have a wait in the parent process. 

    default:
        sleep(T_SLP_P);
        printf("--P (%d): Woke up. I have created child process with ID (%d)\n", getpid(), id);
        printf("--P (%d): Exiting...\n", getpid());

    }

    return 0;
}