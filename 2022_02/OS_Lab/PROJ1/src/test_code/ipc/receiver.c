#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
// for Message queue
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#define BUFFER_SIZE 1024

typedef struct {
    long msgtype;
    int value;
    char buf[BUFFER_SIZE];
} msgbuf;

int main() {
    int key_id;
    msgbuf msg;
    msg.msgtype = 1;
    
    key_id = msgget((key_t) 1234, IPC_CREAT|0666); // Create Message (message queue key, message flag)
    
    if (key_id == -1) {
        printf("Message Get Failed!\n");
        exit(0);
    }
    
    while (1) {
        if (msgrcv(key_id, &msg, sizeof(msg), 1, 0) == -1) { // Receive if msgtype is 1
            printf("Message Receiving Finished!\n");
            exit(0);
        }
        
        printf("value: %d\n", msg.value);
    }

    printf("Message Receiving Finished!\n");
    exit(0);
}
