/*--------------Notes--------------*/
/*
    1) To properly test whether it catches signals or not, using VSCode's terminal or Z-shell isn't suitable since they intercept the commands?
    To do that properly, launch the program in one Z-shell, then use kill commands to send signals to a process like "kill SIGINT <pid>"

    2) It's a good practice to not to override the default action of the signal. After handling the signal, we should let it finally do it's default action.

    */

/*--------------Libraries and Headers--------------*/
#define _POSIX_C_SOURCE 200809L // for sigaction. This tells the compiler to expose the full POSIX API, including struct sigaction.
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

/*--------------Enumerations and Settings--------------*/
enum BUFFER_SIZES
{
    SIZE_BUF_LOG = 64,
};

enum FD
{
    FD_STDIN = 0,
    FD_STDOUT = 1,
    FD_STDERR = 2
};

/*--------------Function Prototypes--------------*/

static void signal_handler(int signo);
static void printer(const char *msg, ...);

/*--------------Flags, Volatiles and Globals--------------*/

static volatile sig_atomic_t caught_sigint = 0; // sig_atomic_t guarantees that read/write operations complete atomically (cannot be interrupted).
static volatile sig_atomic_t caught_sigterm = 0;
// With plain int, on some CPUs (especially 16-bit or 32-bit systems with alignment restrictions), increment may need multiple instructions, making it unsafe.

/*--------------Main Function--------------*/

int main(void)
{
    printer("(%d) : Started executing.", getpid());

    // Initially, block signals to prevent race conditions or missed signals from other processes, keep the signals pending until you're ready to catch!!

    sigset_t emptymask, blockmask;
    sigemptyset(&emptymask);
    sigemptyset(&blockmask);
    sigaddset(&blockmask, SIGINT);
    sigaddset(&blockmask, SIGTERM);
    if (sigprocmask(SIG_BLOCK, &blockmask, NULL) == -1)
    {
        printer("-C (%d) : Error: sigprocmask");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    sa.sa_handler = signal_handler; // Set signal handler function
    // sa.sa_handler = SIG_IGN; // to ignore the handled signals
    // sa.sa_handler = SIG_DFL; // to let the handled signals do their default job
    sigemptyset(&sa.sa_mask); // We kept sa_mask empty, so no signals are blocked while handler runs.
    sa.sa_flags = 0;          // No special flags (e.g., SA_RESTART), so system calls like read() or pause() are interrupted by the signal.
    // sa.sa_flags = SA_RESTART     // Optional, would make certain syscalls automatically restart if interrupted

    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        printer("Cannot handle SIGINT");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1)
    {
        printer("Cannot handle SIGTERM");
        exit(EXIT_FAILURE);
    }

    printer("(%d) : I'm ready to catch. Send SIGINT or SIGTERM to me.", getpid());

    while (!(caught_sigint || caught_sigterm))
    {
        // ---- You can do other non-blocking tasks until sigsuspend----

        sigsuspend(&emptymask); // temporarily sets the process signal mask to mask (empty (allow all signals)), then sleeps until a signal arrives, then restores the original mask automatically.
        // After sigsuspend() returns, it wakes up and executes below commands.
        // This approach avoids the race condition that pause() had. With sigsuspend(), the mask and sleeping are atomic, no signal is lost.

        if (caught_sigint)
        {
            caught_sigint = 0;
            printer("(%d) : Caught SIGINT, i'll continue catching", getpid());
        }
        if (caught_sigterm)
        {
            caught_sigterm = 0;
            printer("(%d) : Caught SIGTERM, now i'm exiting", getpid());
            break;
        }
    }

    exit(EXIT_SUCCESS);
}

/*--------------Function Implementations--------------*/

static void signal_handler(int signo)
{
    // Do async-safe and basic operations here
    switch (signo)
    {
    case SIGINT:
        caught_sigint = 1;
        break;

    case SIGTERM:
        caught_sigterm = 1;
        break;
    default:
        break;
    }
}

static void printer(const char *msg, ...)
{
    char buf_log[SIZE_BUF_LOG] = {'\0'};
    va_list args;

    va_start(args, msg);

    int bytes_written_to_buf = vsnprintf(buf_log, sizeof(buf_log), msg, args);
    va_end(args);

    if (bytes_written_to_buf > 0)
    {
        if (bytes_written_to_buf > SIZE_BUF_LOG)
            bytes_written_to_buf = SIZE_BUF_LOG; // truncate if too long

        (void)write(FD_STDOUT, buf_log, bytes_written_to_buf);
        (void)write(FD_STDOUT, "\n", 1);
    }
}