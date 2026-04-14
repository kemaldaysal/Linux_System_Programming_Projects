/* ---- Process 1 ---- */

/* --- TO DO ---

- Build a synchronization primitive or a second sync semaphore to ensure unlink is made after 
everyone is done using it.

*/

/* Notes

- Both processes that are using the semaphore must open 
the semaphore with RDWR permissions
- Created semaphore is stored in /dev/shm/ until unlinking it!!
- Even when unlinked and disappeared from the dev/shm/, 
the semaphore object itself stays alive as long as at least one
process still has it open, so process 2 and 3 can still use it.
- The process that owns the semaphore lifetime policy should unlink it exactly once.
The common and safest design is, creator tracks the semaphore usage by 
  other processes, and when everything is done, creator should do sem_close()
  sem_unlink(). This is the cleanest approach when there is a clear "manager" process.
Second unlink-close strategy is the creator unlinks immediately after succesful creation/open,
 but this is only safe if all needed processes inherit or already obtained access 
 before unlink matters, or the design does not require late joiners.
 For indepentently started processes that open by name later, this pattern is usually not suitable.
-If P1, P2, P3 are launched manually from separate terminals, then P1 has no built-in way to know whether
P2 and P3 are done just by looking at the semaphore.
In that case, if you still want P1 to be unlinker, you need an explicit protocol, such as:
shared memory user counter,
another semaphore/event to signal completion,
pipes,
message queue,
PID tracking plus waitpid() only if they are children.
Without such a protocol, P1 cannot safely know when others are done.

- Each process should generally
 do sem_close(sem) after finishing using it,
since that closes it's own reference
- Every process that called sem_open() should call sem_close()
- Only one process (preferably the creator, or the last user/cleaner) should call sem_unlink()

- sem_wait() blocks while sval == 0 (out of resource(s) (sem(s))) (until sval > 0) 
- sem_wait() returns immediately if sval > 0 (resource(s) (sem(s)) are available)
- sem_wait() decrements sval by 1 (consumes 1 resource (sem))

- sem_post() increments sval by 1 (frees 1 resource (sem))

- By adjusting SEM_INITIAL_VALUE, setting how many processes can 
execute the critical section paralelly is possible
1 means only one process can execute the critical section
2 means 2 processes can execute the critical section at the same time, 
  3rd process must wait depending on timing 

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
#define FILE_MODES (O_RDWR | O_CREAT)
#define FILE_PERMISSIONS (S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH)
// #define SEM_INITIAL_VALUE 1 // 2 ()
#define SEM_INITIAL_VALUE 2

#define SEM_DONE_CNT_INITIAL_VALUE 0
#define CNT_JOINERS_EXPECTED 2

#define SLEEP_TIME_S 10

/* ---- Main Function ---- */
int main()
{
    int sval = 0;
    int s_done_cnt_val = 0;
    sem_t *sem = sem_open(SEMAPHORE_NAME, FILE_MODES, FILE_PERMISSIONS, SEM_INITIAL_VALUE);
    sem_t *sem_done_cnt = sem_open(SEMAPHORE_DONE_CNT_NAME, FILE_MODES, FILE_PERMISSIONS, SEM_DONE_CNT_INITIAL_VALUE);

    if ((sem == SEM_FAILED) || (sem_done_cnt == SEM_FAILED))
    {
        perror("sem_open failed");
        exit(EXIT_FAILURE);
    }

    printf("P1(%d): Semaphore opened/created successfully.\n", getpid());

    sem_getvalue(sem, &sval); // 1 (SEM_INITIAL_VALUE)
    printf("P1(%d): Obtained semaphore value before sem_wait: %d\n", getpid(), sval);
    // -------------- Critical section - start
    sem_wait(sem); // wait state
    // after executing, sval = 1 - 1 = 0
    printf("P1(%d): Access gained\n", getpid());
    sem_getvalue(sem, &sval); // 0
    printf("P1(%d): Obtained semaphore value after sem_wait: %d\n", getpid(), sval);

    printf("P1(%d): Executing critical section\n", getpid());

    printf("P1(%d): Starting to sleep for %d seconds.\n", getpid(), SLEEP_TIME_S);
    sleep(SLEEP_TIME_S);
    printf("P1(%d): Critical section finished executing, releasing semaphore resource...\n", getpid());
    sem_post(sem); // 0 + 1 = 1
    // -------------- Critical section - end

    sem_getvalue(sem, &sval); // 1
    printf("P1(%d): Obtained semaphore value after sem_post: %d\n", getpid(), sval);

    printf("P1(%d): Before closing and unlinking, waiting for other sempahore users to post sem_done_cnt...\n", getpid());

    for (int i = 0; i < CNT_JOINERS_EXPECTED; ++i)
    {
        if (sem_wait(sem_done_cnt) == -1)
        {
            perror("sem_wait sem_done_cnt failed");
            exit(EXIT_FAILURE);
        }
    }

    sem_getvalue(sem_done_cnt, &s_done_cnt_val);
    
    printf("P1(%d): sem_done_cnt reached %d, closing and unlinking semaphore...\n", getpid(), s_done_cnt_val);    
    sem_close(sem_done_cnt);
    sem_close(sem);
  
    sem_unlink(SEMAPHORE_DONE_CNT_NAME);
    sem_unlink(SEMAPHORE_NAME);

    exit(EXIT_SUCCESS);
}