/* ---------- Libraries and Includes ---------- */

#define _POSIX_C_SOURCE 200809L // for sigaction. This tells the compiler to expose the full POSIX API, including struct sigaction.
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
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

static volatile sig_atomic_t got_sigusr1 = 0;

/* ---------- Main Function ---------- */

int main()
{
    printer("-C (%d) : Switched to Program_2", getpid());

    sigset_t blockmask, emptymask;
    sigemptyset(&emptymask);
    sigemptyset(&blockmask);
    sigaddset(&blockmask, SIGUSR1);
    if (sigprocmask(SIG_BLOCK, &blockmask, NULL) == -1)
    {
        printer("-C (%d) : Error: sigprocmask");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGUSR1, &sa, NULL) == -1)
    {
        printer("-C (%d) : Can't handle SIGUSR1", getpid());
        exit(EXIT_FAILURE);
    }

    printer("-C (%d) : Notifying the parent that i'm ready with SIGUSR1.", getpid());
    kill(getppid(), SIGUSR1);

    printer("-C (%d) : I'm waiting for SIGUSR1 from parent", getpid());

    while (!got_sigusr1)
    {
        sigsuspend(&emptymask);
    }

    printer("-C (%d) : Caught SIGUSR1 from parent.", getpid());

    printer("-C (%d) : Responding to parent with SIGUSR2 signal.", getpid());
    if (kill(getppid(), SIGUSR2) == -1)
    {
        printer("-C (%d) : Kill error");
        exit(EXIT_FAILURE);
    }

    printer("-C (%d) : Exiting", getpid());

    exit(EXIT_SUCCESS);
}

/* ---------- Function Implementations ---------- */

static void signal_handler(int signo)
{
    if (signo == SIGUSR1)
    {
        got_sigusr1 = 1;
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