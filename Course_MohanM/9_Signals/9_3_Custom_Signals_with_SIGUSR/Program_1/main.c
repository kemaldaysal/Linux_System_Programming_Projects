/* NOTES

Child starts → sends SIGUSR1 to parent → parent sends SIGUSR1 to child → child responds with SIGUSR2 → parent continues


*/

/* ---------- Libraries and Includes ---------- */

#define _POSIX_C_SOURCE 200809L // for sigaction. This tells the compiler to expose the full POSIX API, including struct sigaction.
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

/* ---------- Enumerations and Defines ---------- */

enum BUFFER_SIZES
{
    SIZE_BUF_LOG = 80,
};

enum FD
{
    FD_STDIN = 0,
    FD_STDOUT = 1,
    FD_STDERR = 2
};

/* ---------- Function Prototypes ---------- */

void printer(const char *msg, ...);
static void signal_handler(int signo);

/* ---------- Flags, Volatiles and Globals ----------*/

static volatile sig_atomic_t got_sigusr2 = 0;
static volatile sig_atomic_t child_is_ready = 0;

/* ---------- Main Function ---------- */

int main()
{
    printer("--P (%d) : Started executing", getpid());

    sigset_t blockmask, emptymask;
    sigemptyset(&emptymask);
    sigemptyset(&blockmask);
    sigaddset(&blockmask, SIGUSR1);
    sigaddset(&blockmask, SIGUSR2);

    if (sigprocmask(SIG_BLOCK, &blockmask, NULL) == -1)
    {
        printer("--P (%d) : Error: sigprocmask");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGUSR1, &sa, NULL) == -1)
    {
        printer("-P (%d) : Can't handle SIGUSR1", getpid());
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGUSR2, &sa, NULL) == -1)
    {
        printer("-P (%d) : Can't handle SIGUSR2", getpid());
        exit(EXIT_FAILURE);
    }

    pid_t cpid = fork();
    switch (cpid)
    {
    case -1:

        exit(EXIT_FAILURE);

    case 0:
        printer("-C (%d) : I've been created, now i'll switch to another program", getpid());
        execl("../Program_2/main", "argtest1", "argest2", NULL);
        printer("This line must not be printed!! execl error happened");
        _exit(EXIT_FAILURE);
    }

    while (!child_is_ready) 
    {
        sigsuspend(&emptymask);
    }

    printer("--P (%d) : Sending SIGUSR1 to child (%d)", getpid(), cpid);
    if (kill(cpid, SIGUSR1) == -1)
    {
        printer("--P (%d) : Kill error");
        exit(EXIT_FAILURE);
    }
    
    printer("--P (%d) : I'm waiting for SIGUSR2 from my child", getpid());
    while (!got_sigusr2)
    {
        sigsuspend(&emptymask);
    }

    printer("--P (%d) : Caught SIGUSR2 from child.", getpid());

    int status = 0;
    pid_t w_pid = waitpid(cpid, &status, 0);
    if (w_pid == -1)
    {
        printer("--P (%d) : Wait failed for child!!");
        exit(EXIT_FAILURE);
    }
    if (WIFEXITED(status))
    {
        printer("--P (%d) : Wait is completed. Child exited with (%d). Now i'll exit.", getpid(), WEXITSTATUS(status));
    }
    else 
    {
        printer("--P (%d) : Child terminated abnormally", getpid());
    }
    
    exit(EXIT_SUCCESS);
}

/* ---------- Function Implementations ---------- */

static void signal_handler(int signo)
{
    if (signo == SIGUSR1)
    {
        child_is_ready = 1;
    }

    if (signo == SIGUSR2)
    {
        got_sigusr2 = 1;
    }
}

void printer(const char *msg, ...)
{
    va_list args;

    char buf[SIZE_BUF_LOG] = {'\0'};

    va_start(args, msg);
    int bytes_written_to_buf = vsnprintf(buf, sizeof(buf), msg, args);
    va_end(args);

    if (bytes_written_to_buf > 0)
    {
        if (bytes_written_to_buf > SIZE_BUF_LOG)
            bytes_written_to_buf = SIZE_BUF_LOG;

        (void)write(FD_STDOUT, buf, bytes_written_to_buf);
        (void)write(FD_STDOUT, "\n", 1);
    }
}