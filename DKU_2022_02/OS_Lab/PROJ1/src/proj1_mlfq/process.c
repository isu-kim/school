//
// @file : process.c
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that implements functions for process initialization.
//

#include "process.h"


pid_t* pid_arr;
extern int time_tick;


/**
 * The wrapper function that runs everything.
 */
void run() {
    if (queue_count <= 0 || time_tick <= 0) { // Check if any of those values are invalid.
        printf("[-] Queue count and time tick must be greater than 0.\n");
        return;
    }

    printf("[+] Starting job...\n");
    printf("    Total Queue count : %d\n", queue_count - 2);
    printf("    Output file : schedule_dump.txt\n");
    start_processes();
}

/**
 * A function that starts scheduling process.
 * This will generate 10 child processes and call parent process that schedules.
 * @param timeQuantum The time quantum amount for round robin.
 * @param totalTime The total time for running scheduling.
 */
void start_processes() {
    // Store PID values for signal handling in the future.
    pid_arr = (pid_t*) malloc(sizeof(pid_t) * 10);  // Init array.
    memset(pid_arr, 0, sizeof(pid_t) * 10); // Memset 0.

    // Following code will generate 10 child processes.
    // When each child processes were generated, parent process will store PID of each process.
    int i;
    for (i = 0 ; i < 10 ; i++) {
        pid_t pid = fork();
        if (pid == 0) { // This means I am child process.
            printf("[+] Child %d generated with PID %d\n", i, getpid());
            child(); // Call child function for child process.
            break; // If we do not break here, child will fork child recursively.
        } else if (pid == -1) { // This means fork failed. Exit.
            printf("[-] Fork failed: %s\n", strerror(errno));
            printf("[-] Exiting...\n");
            exit(0);
        } else { // This means I am parent process.
            pid_arr[i] = pid;
        }
    }

    // Need to check iteration count, if not we will not know iteration count that this function was called.
    // With this state, it means the parent process generated 10 child processes.
    if (i >= 10) {
        printf("[+] Starting scheduling in 1 second.\n");
        sleep(1);
        parent();
    }
}