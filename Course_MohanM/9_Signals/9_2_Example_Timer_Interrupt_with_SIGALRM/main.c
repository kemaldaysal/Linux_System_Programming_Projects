/* ---------- Libraries and Includes ---------- */

#define _POSIX_C_SOURCE 200809L // for sigaction. This tells the compiler to expose the full POSIX API, including struct sigaction.
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

/* ---------- Enumerations and Defines ---------- */

enum BUFFER_SIZES
{
    SIZE_BUF_LOG = 130
};

enum FD
{
    FD_STDIN = 0,
    FD_STDOUT = 1,
    FD_STDERR = 2
};

enum TIMERS
{
    T_INITIAL = 2,  // seconds
    T_PERIODIC = 1, // seconds
    T_END = 5       // seconds
};

/* ---------- Function Prototypes ---------- */

static void signal_handler(int signo);
static void printer(const char *msg, ...);

/* ----------Flags, Volatiles and Globals---------- */

static volatile sig_atomic_t caught_sigalrm = 0;

/* ---------- Main Function ---------- */

int main()
{
    sigset_t blockmask, emptymask;
    sigemptyset(&emptymask);
    sigemptyset(&blockmask);
    sigaddset(&blockmask, SIGALRM);
    if (sigprocmask(SIG_BLOCK, &blockmask, NULL) == -1)
    {
        printer("-C (%d) : Error: sigprocmask");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGALRM, &sa, NULL) == -1)
    {
        printer("Cannot handle SIGALRM!!");
        exit(EXIT_FAILURE);
    }

    alarm(T_INITIAL); // interrupt is set to trigger after 2 seconds

    printer("(%d) : Setup is done, timer will increment in every %d seconds for %d seconds. The first alarm will be triggered in %d seconds. ", getpid(), T_PERIODIC, T_END, T_INITIAL);

    while (!caught_sigalrm)
    {
        sigsuspend(&emptymask);
        static volatile size_t counter = 0;
        printer("(%d) : Caught SIGALARM, seconds counter: %d", getpid(), counter++);
        if (counter == T_END)
        {
            printer("Threshold (%d seconds) is reached, exiting...", T_END);
            break;
        }

        caught_sigalrm = 0;
        alarm(T_PERIODIC); // set the timer to repeat in each T_PERIODIC second
    }

    exit(EXIT_SUCCESS);
}

/* ---------- Function Implementations ---------- */

static void signal_handler(int signo)
{
    switch (signo)
    {
    case SIGALRM:
        caught_sigalrm = 1;
        break;
    default:
        break;
    }
}

static void printer(const char *msg, ...)
{
    char buf[SIZE_BUF_LOG] = {'\0'};
    va_list args;

    va_start(args, msg);
    int bytes_written_to_buf = vsnprintf(buf, sizeof(buf), msg, args);
    va_end(args);

    if (bytes_written_to_buf > 0)
    {
        if (bytes_written_to_buf > SIZE_BUF_LOG)
            bytes_written_to_buf = SIZE_BUF_LOG;

        write(FD_STDOUT, buf, bytes_written_to_buf);
        write(FD_STDOUT, "\n", 1);
    }
}