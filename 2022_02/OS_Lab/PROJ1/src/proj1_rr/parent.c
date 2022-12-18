//
// @file : parent.c
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that implements functions for parent process which acts like a kernel process.
//

#include "parent.h"

extern int up_msg_q; // The upstream message queue from child to parent.
extern int dn_msg_q; // The downstream message queue from parent to child.
extern pid_t* pid_arr;
extern int time_quantum; // For storing time quantum.
extern int time_tick; // For storing time tick.

int current_ready_queue = QUEUE_READY; // For tracking which queue are we using for the ready queue.
unsigned long total_time_ticks = 0; // For storing how much time ticks had elapsed.

struct pcb_t* running; // The current running process.
FILE* fp = NULL; // For writing files.


/**
 * For graceful killing child processes.
 * @param ignored Will always be SIGINT. so ignore
 */
void exit_handler(int ignored) {
    printf("[Scheduler] Killing all child processes..\n");
    for (int i = 0 ; i < 10 ; i++) {
        printf("[Scheduler] Sending SIGKILL to child %d\n", pid_arr[i]);
        kill(pid_arr[i], SIGKILL);
    }
    printf("[Scheduler] Destroying all message queues.\n");

    print_stats();
    destroy_queues(); // Destroy all queues.
    msgctl(up_msg_q, IPC_RMID, NULL); // Clean IPC message queue with msgctl.
    msgctl(dn_msg_q, IPC_RMID, NULL); // Clean IPC message queue with msgctl.
    exit(0);
}


/**
 * A function that dispatches next process that is in the head of the waiting queue.
 * Then it will pop a new one into the running process.
 * If the popped one was NULL, meaning that it's time for a swap between inactive queue and ready queue, do it.
 * Then it will pop a new one into the running process again.
 * If this was a NULL process again, meaning that all processes were terminated. Then exit.
 */
void dispatch_next() {
    running = pop_queue(current_ready_queue); // Get a new pcb for the running process.
#ifdef DEBUG
    if (running)
        printf("[DEBUG] Scheduler dispatching %d in next tick.\n", running->pid);
#endif
    if (running)
        fprintf(fp, "[Scheduler] Dispatching %d in next tick.\n", running->pid);
    if (!running) { // If running was NULL meaning that the queue was empty, swap ready queue and inactive queue.
#ifdef DEBUG
        printf("[DEBUG] Scheduler swapped inactive queue and ready queue.\n");
#endif
        fprintf(fp, "[Scheduler] Swapped inactive queue and ready queue.\n");
        current_ready_queue = current_ready_queue == QUEUE_READY ? QUEUE_INACTIVE : QUEUE_READY; // Swap queues.
        running = pop_queue(current_ready_queue); // Pop a pcb from the ready queue.
        if (!running) { // If there is no process that is in the ready queue.
            if (queues[QUEUE_WAIT] == NULL) { // This means all processes are terminated.
                printf("[Scheduler] All processes were terminated.\n");
                exit_handler(0); // Call exit handler.
            }
        } else  // This means there was a running process.
            kill(running->pid, SIGUSR1); // Notify the child that you are scheduled.
    } else
        kill(running->pid, SIGUSR1); // Notify the child that you are dispatched.
}


/**
 * A function that manages queues.
 * This function will move current running process into inactive queue.
 */
void time_quantum_over() {
    kill(running->pid, SIGUSR2); // Send the process that time quantum was over.
    move_inactive_queue(running); // Move current running process into inactive queue.
    dispatch_next(); // Dispatch next one from the ready queue.
}


/**
 * A function that moves current running process into the wait queue.
 */
void move_wait_queue() {
#ifdef DEBUG
    printf("[DEBUG] Moving pid %d to wait queue / IO : %d\n", running->pid, running->io_left);
#endif
    fprintf(fp, "[Scheduler] Moving pid %d to wait queue / IO : %d\n", running->pid, running->io_left);
    kill(running->pid, SIGUSR2); // Send the process you are not using CPU time anymore.
    push_queue(QUEUE_WAIT, *running); // Push into the wait queue.
    dispatch_next(); // Dispatch next one from the ready queue.
}


/**
 * A callback function when timer tick was fired.
 * In each timer tick, the parent will check if current process had its time quantum left.
 * If the time quantum was over, it will pop a new process from ready queue.
 * Meanwhile, the running process will be moved into the inactive queue.
 * If the popped process was NULL, meaning that ready queue was empty, it will swap ready queue and inactive queue.
 * @param ignored This will be always SIGALRM. So ignore the argument.
 */
void parent_timer_handler(int ignored) {
    total_time_ticks++; // Increment total time tick.

    if (running) { // There is running process at the moment.
#ifdef DEBUG
        system("clear");
        printf("=============== [ Tick %lu ] ===============\n", total_time_ticks + 1);
#endif
        kill(running->pid, SIGALRM); // Send time tick to the running process.
        running->total_exec_time++; // Increment total CPU time.
        running->r_tq--; // Decrement a time tick.
        if (running->r_tq <= 0) // If current process had its time tick over, move into inactive queue.
            time_quantum_over(); // This means time quantum was over.
        // We will be sending a timer tick from parent to child process to let child know about time passage.
        // If we make another timer tick in the child process as well, there might be potential of mismatching time ticks
        // between parent and child processes.
#ifdef DEBUG
        printf("[DEBUG] Sent time tick to child processes %d\n", running->pid);
#endif
        decrement_all_io_time(); // Decrement all IO time in wait queue.
        check_ipc(); // Check IPC message queue.
        if (total_time_ticks <= 10000) // log time ticks up to 10000
            log_status(); // Log status into the designated file.
#ifdef DEBUG
        printf("[DEBUG] ");
        char rq[MAX_STRING];
        char wq[MAX_STRING];
        char iq[MAX_STRING];
        queue_to_str(current_ready_queue, rq);
        wait_queue_to_str(wq);
        queue_to_str(current_ready_queue == QUEUE_READY ? QUEUE_INACTIVE : QUEUE_READY, iq);
        printf("Ready Queue : %s\n        Wait Queue : %s\n        Inactive Queue : %s\n", rq, wq, iq);

        if (running) {
            printf("Running : %d\n", running->pid);
            printf("        Remaining Time Quantum : %d\n", running->r_tq);
        } else {
            printf("No running processes.\n");
        }
#endif
    } else { // There is no process running at the moment.
#ifdef DEBUG
        system("clear");
        printf("=============== [ Tick %lu ] ===============\n", total_time_ticks + 1);
#endif
        if (queues[current_ready_queue]) { // If the ready queue has something,
            running = pop_queue(current_ready_queue); // Pop one from the ready queue.
            total_time_ticks--;
            kill(getpid(), SIGALRM); // Call handler again.
            return;
        } else {
            // There is no running process at the moment. This means there is IO waiting in the queue.
            if (queues[QUEUE_WAIT]) { // If wait queue was not empty, meaning that there is IO job waiting.
                decrement_all_io_time(); // Decrement all IO time in wait queue.
                if (total_time_ticks <= 10000) // log time ticks up to 10000
                    log_status(); // Log status into the designated file.
#ifdef DEBUG
                printf("[DEBUG] ");
                    char rq[MAX_STRING];
                    char wq[MAX_STRING];
                    char iq[MAX_STRING];
                    queue_to_str(current_ready_queue, rq);
                    wait_queue_to_str(wq);
                    queue_to_str(current_ready_queue == QUEUE_READY ? QUEUE_INACTIVE : QUEUE_READY, iq);
                    printf("Ready Queue : %s\n        Wait Queue : %s\n        Inactive Queue : %s\n", rq, wq, iq);
                    printf("No running processes.\n");
#endif
            } else { // This means all processes and wait queues were over.
                printf("[Scheduler] All processes were terminated.\n");
                exit_handler(0); // Call exit handler.
            }
        }
    }
}


/**
 * A function that logs status of everything into the log file.
 */
void log_status() {
    fprintf(fp, "=============== [ Tick %lu ] ===============\n", total_time_ticks + 1);
    char rq[MAX_STRING];
    char wq[MAX_STRING];
    char iq[MAX_STRING];
    queue_to_str(current_ready_queue, rq);
    wait_queue_to_str(wq);
    queue_to_str(current_ready_queue == QUEUE_READY ? QUEUE_INACTIVE : QUEUE_READY, iq);
    fprintf(fp, "Ready Queue : %s\n        Wait Queue : %s\n        Inactive Queue : %s\n", rq, wq, iq);
    if (running) {
        fprintf(fp, "Running : %d\n", running->pid);
        fprintf(fp, "        Remaining Time Quantum : %d\n", running->r_tq);
    } else {
        fprintf(fp, "No running processes.\n");
    }
}


/**
 * A function that checks if IPC message queue had something in it.
 * This will check if the child process requested an IO burst.
 * If there was an IO burst, schedule the running process out into the wait queue.
 */
void check_ipc() {
    struct msg_q_data_t msg;
    memset(&msg, 0, sizeof(msg));
    long res = msgrcv(up_msg_q, &msg, sizeof(struct msg_q_data_t), 0, IPC_NOWAIT);
    if (res != -1) { // Meaning that some child process issued an io request.
        pid_t pid = msg.pid;
        running->state = STATE_WAIT;
        unsigned int io_time = msg.io_request;
        int finished = msg.is_finished;
        kill(SIGUSR2, pid); // Make process wait.
#ifdef DEBUG
        printf("[DEBUG] Scheduler process received IO request from %d for duration %d / Terminated %d\n", pid, io_time, finished);
#endif
        fprintf(fp, "[Scheduler] Received IO request from %d for duration %d / Terminated %d\n", pid, io_time, finished);
        running->io_left = io_time; // Set required IO time from message queue.
        running->total_io_time += io_time;
        if (finished)  // If the process was terminated process, mark it terminated.
            running->state = STATE_TERMINATED;
        if (io_time) // If io time was not 0, put it into the wait queue.
            move_wait_queue(); // Manage queues.
    }
}


/**
 * A function that decrements all IO time from wait queue.
 * This also will do IO boost when a process had its IO time over.
 */
void decrement_all_io_time() {
    struct pcb_t* tmp = queues[QUEUE_WAIT];
    struct pcb_t* before = NULL;

    while (tmp) { // Iterate over all PCBs and decrement an io tick.
        tmp->io_left--;
        if (tmp->io_left <= 0) { // If IO wait was over, do IO boost.
            if (!before) queues[QUEUE_WAIT] = tmp->next; // If this was start element, set next as start.
            else before->next = tmp->next; // If this was an element in the middle, just un-link this element.

            if (tmp->state == STATE_TERMINATED) { // If the process was terminated, just remove it from the queue.
                printf("[Scheduler] Child %d got terminated at %lu\n", tmp->pid, total_time_ticks);
                fprintf(fp, "[Scheduler] Child %d got terminated at %lu\n", tmp->pid, total_time_ticks);
                tmp->terminated_at = total_time_ticks; // Store when the process got terminated.
                push_queue(QUEUE_TERMINATED, *tmp); // Push current process into the terminated queue.
            } else { // If the process was not terminated.
#ifdef DEBUG
                printf("[DEBUG] PID %d finished IO wait. Performing IO burst!\n", tmp->pid);
#endif
                fprintf(fp, "[Scheduler] PID %d finished IO wait. Performing IO burst!\n", tmp->pid);
                push_queue_head(current_ready_queue, *tmp); // Then add the IO boosted one to the head of the queue.
                tmp->r_tq = time_quantum; // Refill time quantum for the IO boosted one.
            }
        } else
            before = tmp;
        tmp = tmp->next;
    }
}


/**
 * A function that moves a target pcb into inactive queue.
 * This will also refill the pcb with the time quantum.
 * @param target The target pcb to put it into the inactive queue.
 */
void move_inactive_queue(struct pcb_t* target) {
#ifdef DEBUG
    printf("[DEBUG] Moving pid %d to inactive queue\n", target->pid);
#endif
    fprintf(fp, "[Scheduler] Moving pid %d to inactive queue\n", target->pid);
    target->r_tq = time_quantum; // Refill time quantum.
    if (running->state != STATE_TERMINATED)
        push_queue(current_ready_queue == QUEUE_READY ? QUEUE_INACTIVE : QUEUE_READY, *target); // Push into inactive queue.
    free(target); // Need to free this since this was a malloced variable.
}


/**
 * A function that prints out status of
 * - Turn around time.
 * - Waiting time.
 */
void print_stats() {
    struct pcb_t* tmp = queues[QUEUE_TERMINATED];
    unsigned long total_turn_around = 0;
    unsigned long total_wait_time = 0;
    printf("=============== [Stats] ===============\n");
    while(tmp != NULL) {
        unsigned long turn_around = tmp->terminated_at; // Since all processes started at the same time 0.
        unsigned long wait_time = tmp->terminated_at - tmp->total_exec_time - tmp->total_io_time; // Calculate wait time.
        total_turn_around += turn_around;
        total_wait_time += wait_time;
        printf("PID: %d\n     Execution Time : %lu\n     IO Time : %lu\n     Turn around time : %lu\n     Wait time: %lu\n",
               tmp->pid, tmp->total_exec_time, tmp->total_io_time, turn_around, wait_time);

        tmp = tmp->next;
    }
    printf("Average turn around time : %lu\nAverage wait time : %lu\n", total_turn_around / 10, total_wait_time / 10);
}


/**
 * A function for parent process.
 * This will run forever until all the processes are over.
 */
_Noreturn void parent() {
    printf("[Scheduler] PID : %d and Time tick : %d\n", getpid(), time_tick);

    // Register Signal Handlers.
    signal(SIGINT, exit_handler); // For exiting process gracefully.

    // Register Timer ticks.
    struct sigaction sa;
    struct itimerval time_tick_dur;

    // Init sigaction, register sigaction for SIGVTALRM
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &parent_timer_handler;
    sigaction(SIGALRM, &sa, NULL);

    // Set time ticks using time_tick.
    time_tick_dur.it_value.tv_sec = (int) time_tick / 1000; // Set second
    time_tick_dur.it_value.tv_usec = (time_tick % 1000) * 1000; // Set microsecond (1000 microseconds = 1 millisecond)
    time_tick_dur.it_interval.tv_sec = (int) time_tick / 1000;
    time_tick_dur.it_interval.tv_usec = (time_tick % 1000) * 1000;
    setitimer(ITIMER_REAL, &time_tick_dur, NULL);

    // Generate all PCBs by PID info.
    for (int i = 0; i < 10; i++) {
        // Generate a new PCB.
        struct pcb_t tmp;
        tmp.io_left = 0;
        tmp.r_tq = time_quantum;
        tmp.state = STATE_READY;
        tmp.pid = pid_arr[i];
        tmp.total_io_time = 0;
        tmp.total_exec_time = 0;

        push_queue(QUEUE_READY, tmp); // Push it into the queue.
    }

    fp = fopen("schedule_dump.txt", "w");

    running = pop_queue(QUEUE_READY); // Pop a process from ready queue.
    kill(running->pid, SIGUSR1); // Send signal that you are dispatched.
    while (1); // Do nothing.
}