//
// @file : parent.c
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that implements functions for parent process which acts like a kernel process.
//

#include "parent.h"

extern int up_msg_q; // The upstream message queue from child to parent.
extern int dn_msg_q; // The downstream message queue from parent to child.
extern pid_t* pid_arr;
extern int time_tick; // For storing time tick.

int current_selected_queue = 2; // For tracking which level of queue we are using right now.
unsigned long total_time_ticks = 0; // For storing how much time ticks had elapsed.
struct pcb_t* running; // The current running process.
FILE* fp = NULL; // For writing files.
int* time_qs; // For storing values of each time quantum for queues.


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
 * A function that moves current running process into the wait queue.
 */
void move_wait_queue() {
#ifdef DEBUG
    printf("[DEBUG] Moving pid %d to wait queue / IO : %d\n", running->pid, running->io_left);
#endif
    fprintf(fp, "[Scheduler] Moving pid %d to wait queue / IO : %d\n", running->pid, running->io_left);
    running->queue_stat = current_selected_queue; // Store the queue where it was at.
    kill(running->pid, SIGUSR2); // Send the process you are not using CPU time anymore.
    push_queue(QUEUE_WAIT, *running); // Push into the wait queue.
    dispatch_next(); // Dispatch next process.
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
            } else { // This means the process finished IO burst, go back to the queue where it should be.
#ifdef DEBUG
                printf("[DEBUG] PID %d finished IO wait. Moving it back to queue %d\n", tmp->pid,  tmp->queue_stat);
#endif
                fprintf(fp, "[Scheduler] PID %d finished IO wait. Moving it back to queue %d\n", tmp->pid, tmp->queue_stat);
                push_queue(tmp->queue_stat, *tmp); // Move it back to the queue where it was at.
                tmp->r_tq = time_qs[tmp->queue_stat - 2]; // Refill time quantum according to the queue where it was at.
            }
        } else
            before = tmp;
        tmp = tmp->next;
    }
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
 * A function that checks if there was an update in the queues.
 * This function will check if the higher priority queues got any elements.
 * If there were elements in higher priority queue, it will immediately stop current running process.
 * Then it will start executing the element in the highest priority queue.
*/

void update_queue_stats() {
    int before_queue = current_selected_queue;
    for (int i = 2 ; i < queue_count ; i++) { // Iterate over all levels of queues.
        if (get_queue_size(i) != 0 && i < current_selected_queue) // If the queue was not empty & it had higher priority
            current_selected_queue = i; // Update the current selected queue.
    }

    if (before_queue != current_selected_queue) { // If there was an update in the current selected queue, stop current.
        kill(running->pid, SIGUSR2); // Stop current running process immediately.
        push_queue(before_queue, *running); // Store old one back to the queue where it was.
        running = pop_queue(current_selected_queue); // Pop the higher most priority process and let it have CPU.
        running->r_tq = time_qs[current_selected_queue - 2]; // Refill time quantum for the dispatched process.
        kill(running->pid, SIGUSR1);
    }
}


/**
 * A function that performs priority boost.
 * This will increment a waiting time for all processes in ready status.
 * If it reaches more waiting time than S, it will be priority boosted.
 */
void perform_priority_boost() {
    for (int i = 3 ; i < queue_count ; i++) { // Start from 2nd priority queue.
        struct pcb_t* tmp = queues[i];
        struct pcb_t* before = NULL;
        while (tmp) { // Iterate over all elements in level.
            tmp->wait_time++;
            // If it waited more than S and is not current running process, then perform priority boost.
            if (tmp->wait_time > S && tmp->pid != running->pid) {
                tmp->wait_time = 0; // Reset wait time.
#ifdef DEBUG
                printf("[DEBUG] Process %d in level %d got priority boosted.\n", tmp->pid, i - 2);
#endif
                if (!before) queues[i] = tmp->next; // If this was start element, set next as start.
                else before->next = tmp->next; // If this was an element in the middle, just un-link this element.
                tmp->r_tq = time_qs[0]; // Set time quantum for the topmost queue.
                push_queue(2, *tmp); // Move this process to the topmost queue.
            } else
                before = tmp;
            tmp = tmp->next;
        }
    }
}


/**
 * A function that processes running process when it had its time quantum over.
 * This function will move running process and schedule next one accordingly.
 */
void time_quantum_over() {
    int dest = current_selected_queue == queue_count - 1 ? current_selected_queue : current_selected_queue + 1;
#ifdef DEBUG
    printf("[DEBUG] Demoting pid %d to queue from %d to %d\n", running->pid, current_selected_queue - 2, dest - 2);
#endif
    push_queue(dest, *running); // Push into the destination queue.
    kill(running->pid, SIGUSR2); // Make the process stop.
    dispatch_next(); // Dispatch next process from the queue.
}


/**
 * A function that handles every time tick interrupt for kernel process.
 * 1. This will update queue status.
 * 2. Send time tick to running process and decrement time quantum.
 * 3. Perform priority boost if needed.
 * 4. Check IPC.
 * @param ignored SIGALRM, so just ignore this.
 */
void parent_timer_handler(int ignored) {
    total_time_ticks++;
    decrement_all_io_time(); // Decrement all IO time.
#ifdef DEBUG
    printf("=============== [ Tick %lu ] ===============\n", total_time_ticks);
    for (int i = 0 ; i < queue_count - 2 ; i++) {
        char tmp[MAX_STRING];
        queue_to_str(i + 2, tmp);
        printf("[%c] Level %d: %s (%d)\n",  current_selected_queue - 2 == i ? '>' : ' ', i, tmp, get_queue_size(i + 2));
    }
    printf("Current Level : %d\n", current_selected_queue - 2);
    char tmp[MAX_STRING];
    wait_queue_to_str(tmp);
    printf("Wait Queue: %s (%d)\n", tmp, get_queue_size(QUEUE_WAIT));
#endif
    if (running) { // If there was running process, check queue status.
        update_queue_stats(); // Might set running as NULL, check conditions!
        kill(running->pid, SIGALRM); // Send time tick to the running process.
        running->total_exec_time++;
        running->r_tq--;
        perform_priority_boost();
        if (running->r_tq <= 0)
            time_quantum_over();
        check_ipc(); // Check IPC communication.
#ifdef DEBUG
        if (running)
            printf("Running : %d (%d)\n", running->pid, running->r_tq);
#endif
    } else { // If there was no running process, there might be two cases.
#ifdef DEBUG
        printf("No running processes\n");
#endif
        printf("queue size :%d\n", get_queue_size(QUEUE_WAIT));
        if (get_queue_size(QUEUE_WAIT) == 0) { // If there is no IO waiting process, meaning that all processes were terminated, exit scheduling.
            printf("All processes terminated.\n");
            exit_handler(0);
        } else { // If there is no IO waiting process, meaning that all processes were terminated, exit scheduling.
            decrement_all_io_time();
        }
    }
    log_status();
}


/**
 * A function that dispatches next process.
 *
 */
void dispatch_next() {
    struct pcb_t* tmp = NULL;
    if (get_queue_size(current_selected_queue) != 0) // If current queue had a process in it, pop from it.
        running = pop_queue(current_selected_queue);
    else { // If current queue did not have any process, search next.
        while (!tmp) { // If current selected queue did not have any element, keep searching until you find one.
            current_selected_queue++;
            if (current_selected_queue >= queue_count)
                break; // If this was the last queue and still did not find one break.
            tmp = pop_queue(current_selected_queue);
        }
        running = tmp; // Store tmp to the running process.
    }
    if (running) {
        running->wait_time = 0; // Reset waiting time when being scheduled.
        running->r_tq = time_qs[current_selected_queue - 2];
        running->queue_stat = current_selected_queue;
        kill(running->pid, SIGUSR1);
    }
}


/**
 * A function that logs status of everything into the log file.
 */
void log_status() {
    fprintf(fp, "=============== [ Tick %lu ] ===============\n", total_time_ticks);
    for (int i = 0 ; i < queue_count - 2 ; i++) {
        char tmp[MAX_STRING];
        queue_to_str(i + 2, tmp);
        fprintf(fp, "[%c] Level %d: %s (%d)\n",  current_selected_queue - 2 == i ? '>' : ' ', i, tmp, get_queue_size(i + 2));
    }
    fprintf(fp, "Current Level : %d\n", current_selected_queue - 2);
    char tmp[MAX_STRING];
    wait_queue_to_str(tmp);
    fprintf(fp, "Wait Queue: %s (%d)\n", tmp, get_queue_size(QUEUE_WAIT));

    if (running)
        fprintf(fp, "Running : %d (%d)\n", running->pid, running->r_tq);
    else
        fprintf(fp, "No running processes\n");
}


/**
 * A function for parent process.
 * This will run forever until all the processes are over.
 */
_Noreturn void parent() {
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

    fp = fopen("schedule_dump.txt", "w");

    // Generate random time quantum for each queues.
    time_qs = malloc(sizeof(int) * queue_count);
    memset(time_qs, 0, sizeof(int) * queue_count);
    printf("[Scheduler] PID : %d and Time tick : %d\n", getpid(), time_tick);
    srand(time(NULL));

    for (int i = 0 ; i < queue_count - 2 ; i++) { // Make lower priority queue's time quantum bigger by TIME_TICK_JUMP time ticks.
        if (i > 0)
            time_qs[i] = TIME_TICK_JUMP + time_qs[i - 1];
        else
            time_qs[0] = TIME_TICK_JUMP;
    }

    printf("[Scheduler] PID : %d\n", getpid());
    for (int i = 0 ; i < queue_count - 2 ; i++)
        printf("- Level %d : Time Quantum %d\n", i, time_qs[i]);

    // Generate all PCBs by PID info.
    // For first process, push it into the topmost queue.
    // For other processes, push it in random queue.
    for (int i = 0; i < 10; i++) {
        // Generate a new PCB.
        struct pcb_t tmp;
        tmp.io_left = 0;
        tmp.state = STATE_READY;
        tmp.pid = pid_arr[i];
        tmp.total_io_time = 0;
        tmp.total_exec_time = 0;
        tmp.wait_time = 0;
        if (i == 0) { // For first element, put it into the topmost queue.
            tmp.r_tq = time_qs[0];
            push_queue(2, tmp);
        } else { // For other elements, put it randomly.
            int dest = rand() % (queue_count - 2) + 2;
            tmp.r_tq = time_qs[dest - 2];
            push_queue(dest, tmp); // Push processes into random queue levels.
        }
    }

    running = pop_queue(2);
    kill(running->pid, SIGUSR1);

    fp = fopen("schedule_dump.txt", "w");
    while (1); // Do nothing.
}