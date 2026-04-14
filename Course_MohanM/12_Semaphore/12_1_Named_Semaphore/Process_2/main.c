/* ---- Process 2 ---- */

/* --- Notes ---
- An existing semaphore is opened in this process, 
hence why the less arguments passed to sem_open 
- Every process that called sem_open() should call sem_close()

- Both processes that are using the semaphore must open 
the semaphore with RDWR permissions


*/

/* ---- Headers ---- */
#include <semaphore.h>

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>


/* ---- Settings with Macros ---- */

#define SEMAPHORE_NAME "/sem1"
#define SEMAPHORE_DONE_CNT_NAME "/sem_done_cnt"

#define FILE_MODES (O_RDWR)

#define SLEEP_TIME_S 7

/* ---- Main Function ---- */
int main()
{
    int ret = 0;
    int sval = 0;

    sem_t *sem = sem_open(SEMAPHORE_NAME, FILE_MODES); 
    sem_t *sem_done_cnt = sem_open(SEMAPHORE_DONE_CNT_NAME, FILE_MODES);

    if ((sem == SEM_FAILED) || (sem_done_cnt == SEM_FAILED))
    {
        perror("sem_open failed");
        exit(EXIT_FAILURE);
    }

    printf("P2(%d): Semaphore opened successfully.\n", getpid());

    sem_getvalue(sem, &sval); //
    printf("P2(%d): Obtained semaphore value before sem_wait: %d\n", getpid(), sval);
    printf("P2(%d): I will try to gain access to semaphore if available, if not i'll wait...\n", getpid());
    // -------------- Critical section - start
    ret = sem_wait(sem); // wait state
    // after executing, sval = 1 - 1 = 0
    printf("P2(%d): Access gained\n", getpid());
    sem_getvalue(sem, &sval); // 0
    printf("P2(%d): Obtained semaphore value after sem_wait: %d\n", getpid(), sval);

    printf("P2(%d): Executing critical section\n", getpid());
    printf("P2(%d): Starting to sleep for %d seconds.\n", getpid(), SLEEP_TIME_S);
    sleep(SLEEP_TIME_S);
    printf("P2(%d): ret: %d\n", getpid(), ret);
    sem_post(sem); // 0 + 1 = 1
    // -------------- Critical section - end

    printf("P2(%d): Critical section finished executing\n", getpid());
    sem_getvalue(sem, &sval); // 1
    printf("P2(%d): Obtained semaphore value after sem_post: %d\n", getpid(), sval);
    printf("P2(%d): Notifying that i'm done by posting sem_done_cnt synchroniser.\n", getpid());    
    sem_post(sem_done_cnt);

    printf("P2(%d): Closing semaphore on my side...\n", getpid());    
    sem_close(sem_done_cnt);    
    sem_close(sem);


    exit(EXIT_SUCCESS);
}