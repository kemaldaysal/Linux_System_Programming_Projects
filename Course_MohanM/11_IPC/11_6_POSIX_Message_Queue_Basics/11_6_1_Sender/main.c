

/* ---- Headers ---- */

#define _POSIX_C_SOURCE 200809L // for sigaction

#include <mqueue.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

/* ---- Settings with Macros ---- */

#define MESSAGE_QUEUE_NAME "/message_queue"
#define FILE_MODES (O_WRONLY | O_CREAT)
#define FILE_PERMISSIONS (S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH)
#define MSG_PRIORITY 0 // 0 is the least priority

/* ---- Globals ---- */

static volatile sig_atomic_t stop_flag = 0;

/* ---- Function Prototypes ---- */

void signal_handler(int sig);

/* ---- Main Function ---- */

int main()
{
    printf("S (%d) : Started executing...\n", getpid());

    struct sigaction sa = {0};
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);

    mqd_t mqd = mq_open(MESSAGE_QUEUE_NAME, FILE_MODES, FILE_PERMISSIONS);
    if (mqd == (mqd_t)-1)
    {
        perror("S : Error: mq_open");
        exit(EXIT_FAILURE);
    }

    struct mq_attr attributes;
    if (mq_getattr(mqd, &attributes) == -1)
    {
        perror("S : Error: mq_getattr");
        mq_close(mqd);
        exit(EXIT_FAILURE);
    }

    printf("S : You can now type messages (max length %ld). Press Ctrl+C to exit.\n",
           attributes.mq_msgsize - 1);

    while (!stop_flag)
    {
        printf("S : Enter the message you want to send: ");
        fflush(stdout);

        char buf_write[attributes.mq_msgsize];

        if (!(fgets(buf_write, sizeof(buf_write), stdin)))
        {
            if (feof(stdin)) // EOF (Ctrl + D)
            {
                break;
            }
            perror("S : Error: fgets");
            break;
        }

        buf_write[strcspn(buf_write, "\n")] = '\0'; // Strip trailing newline from fgets for edge cases

        if (strlen(buf_write) >= (size_t)attributes.mq_msgsize)
        {
            fprintf(stderr, "S : Error: message too long for queue (max %ld)\n", attributes.mq_msgsize - 1);
            continue;
        }

        if ((mq_send(mqd, buf_write, strlen(buf_write), MSG_PRIORITY)) == -1)
        {
            perror("S : Error: mq_send");
            break;
        }
        printf("S : Message sent successfully\n");
    }
    mq_close(mqd);
    printf("S : Exiting successfully...\n");
    exit(EXIT_SUCCESS);
}

void signal_handler(int sig)
{
    stop_flag = 1;
}