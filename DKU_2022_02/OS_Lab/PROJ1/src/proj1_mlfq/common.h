//
// @file : common.h
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that defines common data structures and common values.
//

#ifndef PROJ1_COMMON_H
#define PROJ1_COMMON_H
#pragma once

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define STATE_READY 1 // For ready state processes.
#define STATE_WAIT 0 // For wait state processes.
#define STATE_TERMINATED 2 // For terminated state processes.
#define MAX_STRING 1024 // For maximum length of string.

// A struct data type for storing information about a process.
struct pcb_t {
    int state; // For storing state of this process.
    pid_t pid; // For storing pid of this process.
    int queue_stat; // For storing where this PCB belonged in the queue.
    unsigned int r_tq; // For storing time quantum of this process.
    struct pcb_t* next; // For storing next element of PCB. This will be like a list.
    unsigned int io_left; // For storing left time ticks for IO operation.
    unsigned long registered_at; // For storing when this process was registered into the scheduler.
    unsigned long terminated_at; // For storing when the process got terminated.
    unsigned long total_exec_time; // For storing total time that this process utilized CPU.
    unsigned long total_io_time; // For storing total time that this process requested IO time.
    unsigned long wait_time; // For storing how much time it spent in wait. This is for Rule 5. in MLFQ
};

// A struct data for storing message queue data.
struct msg_q_data_t {
    int mtype; // For storing message type.
    pid_t pid; // For storing PID info of child process.
    unsigned int io_request; // For storing length of IO request time.
    int is_finished;
};

// A struct data for storing workload from file.
struct workload_t {
    int number_of_queues; // Total number of queues.
    int io_burst_duration; // Total IO burst duration.
    int cpu_burst_duration; // Total CPU burst duration.
    int process_interval; // The interval between new processes.
    int* priority_of_queues; // For storing priority of each queue.
    int* scheduler_of_queues; // For storing scheduler type of ech queue.
    int* time_quantum; // For storing time tick of each queue if this was a RR scheduler.
};

#endif //PROJ1_COMMON_H
