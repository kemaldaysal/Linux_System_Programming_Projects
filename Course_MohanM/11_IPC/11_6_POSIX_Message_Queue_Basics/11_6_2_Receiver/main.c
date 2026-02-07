/* Additional Notes:
    Creation
        -We don't specify the message queue's location since Kernel handles it and stores the message queues in /dev/mqueue.
        -We only specify the message queue's name, and provide the same name on the other (sender) side.

        -To see all the present message queues with their file permissions, view the files inside /dev/mqueue
        -The creator of the message queue (receiver side in here) should also provide an extra parameter providing the attributes of the message queue.
            -Leaving it as NULL makes the attributes default, but in here we have modified it and passed the modified struct.

    Settings and Defaults
        -We can set custom mq_maxmsg and mq_msgsize attributes rather than their default ones (maxmsg: 10, msgsize: 8192 )

    Message-based Blocking Behaviour (default)
    -In default blocking mode, the receiver waits for a 'message' to arrive to proceed.
        In PIPE and FIFO, receiver waits for a sender to connect to the PIPE or FIFO, then it continues.
        But in message queues, waiting is based on the 'message arrived' signal.

    Disconnection of Sender
    -Contrary to FIFO and PIPE (??), the sender can disconnect and connect anytime without causing any problems to the receiver. Receiver can't sense it (???) and it keeps waiting for new messages from any senders.
        -Same thing applies vice-versa, the writer can send it's messages to the message queue even when the reader is disconnected. Reader gets them whenever it goes online, thanks to kernel who is managing all of it.

    Unlinking
        -mq_close() only closes the current message_queue, but the queue still exists in the system under /dev/mqueue/message_queue until you mq_unlink().
        -If you run your program again without unlinking, you’ll just reopen the old queue, which may already have stale messages.

        -The receiver (server) side should normally own unlinking.
        */
/* ---- Headers ---- */

#define _POSIX_C_SOURCE 200809L // for sigaction

#include <mqueue.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

/* ---- Settings with Macros ---- */

#define MESSAGE_QUEUE_NAME "/message_queue"
#define FILE_MODES (O_RDONLY | O_CREAT)
#define FILE_PERMISSIONS (S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH)
// #define OPTION_ATTRIBUTES NULL // Null to pass default attributes
#define NUM_OF_MESSAGES_TO_RECEIVE 1
#define OPTION_RECEIVED_MESSAGE_PRIORITY NULL
#define OPTION_MAXMSG 5
#define OPTION_MSGSIZE 256

/* ---- Globals ---- */
static volatile sig_atomic_t stop_flag = 0;

/* ---- Function Prototypes ---- */

static void cleanup(const mqd_t message_queue_descriptor, char *ptr_to_malloced_location);
void signal_handler(int sig);

/* ---- Main Function ---- */

int main()
{
    printf("R (%d) : Started executing...\n", getpid());

    struct sigaction sa = {0};
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);

    // Always try to unlink first (ignore errors if it didn't exist)
    mq_unlink(MESSAGE_QUEUE_NAME);

    // struct mq_attr attributes; // for default attributes
    struct mq_attr attributes = {0}; // for fresh start with custom valuables, to not waste kernel memory
    attributes.mq_maxmsg = OPTION_MAXMSG;
    attributes.mq_msgsize = OPTION_MSGSIZE;

    mqd_t mqd = mq_open(MESSAGE_QUEUE_NAME, FILE_MODES, FILE_PERMISSIONS, &attributes);
    if (mqd == (mqd_t)-1)
    {
        perror("R : Error: mq_open");
        exit(EXIT_FAILURE);
    }

    if (mq_getattr(mqd, &attributes) == -1)
    {
        perror("R : Error: mq_getattr");
        cleanup(mqd, NULL);
        exit(EXIT_FAILURE);
    }

    printf("R : Initialization successful, displaying attributes...\n");
    printf("R : Message queue mq_maxmsg: %d, mq_msgsize: %d\n", (int)attributes.mq_maxmsg, (int)attributes.mq_msgsize);

    char *msg_buf = malloc(attributes.mq_msgsize); // This buffer will be overridden and re-used in each message
    if (!msg_buf)
    {
        perror("R : Error: malloc");
        cleanup(mqd, msg_buf);
        exit(EXIT_FAILURE);
    }

    printf("R: Entered blocking mode, waiting for a 'MESSAGE' to arrive (not waiting for a sender like in FIFO or PIPE). (CTRL+C to exit)\n");

    while (!stop_flag)
    {
        ssize_t num_of_received_bytes = mq_receive(mqd, msg_buf, attributes.mq_msgsize, OPTION_RECEIVED_MESSAGE_PRIORITY);
        if (num_of_received_bytes == -1)
        {
            if (errno == EINTR)
            {
                if (stop_flag)
                {
                    printf("R: Kill signal caught!\n");
                    break;
                }
                fprintf(stderr, "R : Read interrupted by an unhandled signal, continuing to read...\n");
                continue;
            }

            perror("Error: mq_receive");
            break;
        }

        printf("R: Received message (%zd bytes): %.*s\n", num_of_received_bytes, (int)num_of_received_bytes, msg_buf); // Don't rely on \0 since msg_send won't automatically add it.
    }

    cleanup(mqd, msg_buf);
    printf("R: Exiting...");
    exit(EXIT_SUCCESS);
}

/* ---- Function Implementations ---- */

static void cleanup(const mqd_t message_queue_descriptor, char *ptr_to_malloced_location)
{
    mq_close(message_queue_descriptor);
    mq_unlink(MESSAGE_QUEUE_NAME);
    if (ptr_to_malloced_location != NULL)
    {
        free(ptr_to_malloced_location);
    }
}

void signal_handler(int sig)
{
    stop_flag = 1;
}