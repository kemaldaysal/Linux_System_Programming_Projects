#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

enum MACROS
{
    T_SLP_C1 = 3,
    T_SLP_C2 = 1
};

int main()
{

    int status = 0;
    size_t count_active_childs = 0;

    pid_t cpid_1 = fork();
    switch (cpid_1)
    {
    case -1:
        perror("Fork failed!!\n");
        exit(-1);
    case 0:
        printf("-C1 (%d): Started executing, now i'll sleep for %d seconds...\n", getpid(), T_SLP_C1);
        sleep(T_SLP_C1);
        printf("-C1: Woke up, now i'll exit.\n");
        exit(2);
    default:

        count_active_childs++;

        pid_t cpid_2 = fork();
        switch (cpid_2)
        {
        case -1:
            perror("Fork failed!!\n");
        case 0:
            printf("-C2 (%d): Started executing, now i'll sleep for %d seconds...\n", getpid(), T_SLP_C2);
            sleep(T_SLP_C2);
            printf("-C2: Woke up, now i'll exit.\n");
            exit(1);
        }
        
        count_active_childs++;

        printf("--P (%d): Started executing, now i'll watch my children but i won't wait here...\n", getpid());

        while (count_active_childs != 0)
        {
            pid_t ret_pid = waitpid(-1, &status, WNOHANG);
            switch (ret_pid)
            {
            case -1:
                perror("Error waitpid\n");
                exit(-1);
                break;

            case 0:
                printf("--P: At least one child (%lu) is still active...\n", count_active_childs);
                sleep(1);
                break;

            default:
                count_active_childs--;
                printf("--P: Reaped child: %d, status %d, remaining child count: %lu\n", ret_pid, status, count_active_childs);
                break;
            }
        }

        printf("--P: No child left, parent exiting\n");
    }

    return 0;
}