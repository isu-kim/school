//
// @file : child.c
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that implements functions for child process which acts like a user process.
//

#include "child.h"

extern pid_t parent_pid; // The parent scheduler process's PID.
extern int up_msg_q; // The upstream message queue from child to parent.
extern int time_tick;

int total_exec_time = 0;
int is_child_finished = 0;
int is_running = 0; // For storing if this process is running at the moment.
int cpu_time; // For storing total CPU burst time.
int io_time; // For storing total IO burst time.


/**
 * A function that sends message to parent process.
 * This will send IO burst time through IPC message queue into parent process.
 */
void child_send_msg() {
    // Generate message buffer with IO time.
    struct msg_q_data_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.mtype = 0; // Needs this for receiving data from parent.
    msg.pid = getpid();
    msg.io_request = io_time;
    msg.is_finished = is_child_finished;
    // Send data through message queue and send signal to parent process.
    int res = msgsnd(up_msg_q, &msg, sizeof(struct msg_q_data_t), IPC_NOWAIT);
    if (res == -1) printf("[%d] Could not send upstream message queue : %s\n", getpid(), strerror(errno));
#ifdef DEBUG
    printf("[DEBUG] Child %d sent IPC communication: IO Time: %d / Is Finished: %d\n", getpid(), io_time, is_child_finished);
#endif
    //io_time = rand() % RANDOM_MAX + 1; // Refill IO burst time.
    io_time = 5;
}


/**
 * The function that handles time tick for child process.
 * The time tick signal will be coming from parent process as a SIGALRM signal.
 * @param ignored Will always be SIGALRM. So ignore the value.
 */
void child_timer_handler(int ignored) {
    total_exec_time++;
#ifdef DEBUG
    if (is_running && !is_child_finished)
        printf("[DEBUG] Child %d Timer Ticked! : Total Time Tick %d / Is running %d / CPU Time %d / IO Time %d\n", getpid(), total_exec_time, is_running, cpu_time, io_time);
#endif
    if (is_running && !is_child_finished)
        cpu_time--;

    if (cpu_time <= 0) { // If CPU burst was over, request IO burst.
        is_running = 0;

        if (total_exec_time + io_time > CHILD_MAX_EXECUTION_TIME) { // If IO burst was not enough, just use it as much.
            io_time = CHILD_MAX_EXECUTION_TIME - total_exec_time; // TODO: Bug here!
            io_time = io_time < 0 ? 1 : io_time;
            is_child_finished = 1;
        } else { // If we had enough time for IO burst, just use whole IO burst time.
            total_exec_time +=  io_time;
            //cpu_time = rand() % RANDOM_MAX; // Refill CPU burst time.
            cpu_time = 5; // Refill CPU burst time.
        }
        child_send_msg();
    }
}


/**
 * A function that is for child signal handling.
 * @param sig The signals.
 *            SIGUSR1: Will let this child process know that it has been dispatched by scheduler.
 *            SIGUSR2: Will let this child process know that the time quantum was over.
 */
void child_signal_handler(int sig) {
    if (sig == SIGUSR1) {
#ifdef DEBUG
        printf("[DEBUG] Child %d got SIGUSR1: Dispatched\n", getpid());
#endif
        is_running = 1; // Set current process as running.
    } else if (sig == SIGUSR2) {
#ifdef DEBUG
        printf("[DEBUG] Child %d got SIGUSR2: Stopped\n", getpid());
#endif
        is_running = 0; // Set current process as stopped.
    }
}


/**
 * A function for child process.
 * This will do following things in order.
 *
 * 1. Set random CPU time and IO burst time.
 * 2. Initialize all mutex variables for process.
 * 3. Register Signals and its handlers.
 * 4. Register Timer interrupts and handler.
 * 5. Start thread and join thread.
 */
void child() {
    // Initialize CPU time and IO time randomly.
    srand(time(NULL));
    //cpu_time = rand() % RANDOM_MAX;
    cpu_time = 5;
    //io_time = rand() % RANDOM_MAX + 1; // At least IO time shall be 1 time tick.
    io_time = 5;
    //printf("[%d] child called\n", getpid());
    //printf("[%d] Executing with max execution %d.", getpid(), 10);

    signal(SIGUSR1, (void*) child_signal_handler); // For starting process.
    signal(SIGUSR2, (void*) child_signal_handler); // For stopping process.
    signal(SIGALRM, (void*) child_timer_handler); // For timer tick from parent process.

    while(!is_child_finished);
    child_send_msg(); // Send message to parent process to let it know that this process was over.
}
