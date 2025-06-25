#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

static int glob = 111; // allocated in data segment

/*
    ** Instructor's explanation about same memory addresses in different processes:
    - The address assigned to these variables may or may not be same(depends on memory at time of allocation), 
    these address represent 2 different virtual memory, hence the address even if it's same or not, 
    they are 2 different memory locations, hence stores different values in Parent and child process.

    - See the ChatGPT's answer for more detailed explanation.
    - See the COpy-on-Write (CoW) systems, meaning physical memory is shared until 
    one process writes to it, at which point a separate copy is made

*/

int main()
{
    int i_stack = 222; // allocated in stack segment
    pid_t pid_child;

    switch (pid_child = fork())
    {
    case -1:
        perror("Fork failed\n");
        exit(-1);
    case 0:
        printf("C: %d is returned to me\n", pid_child);
        glob *= 3;
        i_stack *= 3;
        printf("C: PID: %d, glob: %d (@%p), i_stack: %d (@%p)\n", getpid(), glob, &glob, i_stack, &i_stack);

        int test = 5;
        printf("C: test variable created here: %d (@%p)\n", test, &test);
        
        sleep(5);
        break;

    default:
        printf("P: I've created a child with ID (%d)\n", pid_child);
        printf("P: PID: %d, glob: %d (@%p), i_stack: %d (@%p)\n", getpid(), glob, &glob, i_stack, &i_stack); // glob is still 111 in here!!
        // Changes to variables doesn't affect between processes. It only affects it's own process owing to it's own virtual memory
        
        printf("P: test variable created in child: %d (@%p)\n", test, &test); // 0, cant reach to child's memory 
        
        sleep(5);
    }

    return 0;
}