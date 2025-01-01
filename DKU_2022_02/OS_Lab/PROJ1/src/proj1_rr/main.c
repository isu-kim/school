//
// @file : main.c
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that implements the almighty main function.
//

#include "utils.h"
#include "process.h"

#define MSG_Q_KEY 0x2917 & 0x0984 // Initialize key with last 4 digits of school id.

struct pcb_t* queues[4]; // 0 -> Ready Queue, 1 -> Wait Queue, 2 -> Inactive Queue.
int dn_msg_q; // For downstream message queue: from parent to child.
int up_msg_q; // For upstream message queue: from child to parent.
int parent_pid; // Not used currently.
int time_quantum; // For storing time quantum
int time_tick; // For storing time tick

 /**
  * The almighty main function
  * @param argc The argc
  * @param argv The argv
  * @return -1 if something went wrong, 0 if successful.
  */
int main(int argc, char* argv[]) {
    // This shall not happen, when initializing queues failed, return -1.
    if (init_queues() == -1) {
        printf("[ERROR] Initializing queues failed.\n");
        return -1;
    }

     printf("----------=[ Scheduler ]=----------\n");
     if (argc != 3) { // Check arguments, also check if they are valid.
         printf("[-] Usage : ./this_file <Time quantum (ticks)> <Time tick (ms)>\n");
         printf("Ex) ./sched 10 60 \n");
         printf("=> Time quantum 10ticks, time tick 60ms\n");
         return 0;
     }

     // Initialize message queues before forking children processes.
     dn_msg_q = msgget(MSG_Q_KEY, 0666 | IPC_CREAT);
     up_msg_q = msgget(MSG_Q_KEY, 0666 | IPC_CREAT);
     parent_pid = getpid(); // Store parent scheduler's PID.

     // Parse command-line arguments
     time_quantum = atoi(argv[1]);
     time_tick = atoi(argv[2]);
     run(); // Start job!

     return 0;
}
