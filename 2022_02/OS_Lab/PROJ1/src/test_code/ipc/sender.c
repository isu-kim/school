#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// for Message queue
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#define BUFFER_SIZE 1024

typedef struct { // Message buffer structure
  long msgtype; // Message type, must be > 0 with 'long' data type
  int value;
  char buf[BUFFER_SIZE]; // Message data to push in queue
} msgbuf;

int main() {
    int cnt = 0;
    int key_id; // Message queue ID
    msgbuf msg;
    msg.msgtype = 1;

    key_id = msgget((key_t) 1234, IPC_CREAT|0666); // Create Message (message queue key, message flag)

    if (key_id == -1) {
        printf("Message Get Failed!\n");
        exit(0);
    }
    
    while (1) {
        msg.value = ++cnt;
    
        if (cnt >= 10) {
            printf("Message Sending Finished!\n");
            break;
        }

        if (msgsnd(key_id, &msg, sizeof(msg), IPC_NOWAIT) == -1) { // IPC_NOWAIT flag: if no more queu space, fail instead of blocking
            printf("Message Sending Failed!\n");
            exit(0);
        }
        
        printf("value: %d\n", msg.value);
        sleep(1);
    }
    exit(0);
}
