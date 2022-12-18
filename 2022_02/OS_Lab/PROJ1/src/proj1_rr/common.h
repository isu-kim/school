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
    unsigned int r_tq; // For storing time quantum of this process.
    struct pcb_t* next; // For storing next element of PCB. This will be like a list.
    unsigned int io_left; // For storing left time ticks for IO operation.
    unsigned long terminated_at; // For storing when the process got terminated.
    unsigned long total_exec_time; // For storing total time that this process utilized CPU.
    unsigned long total_io_time; // For storing total time that this process requested IO time.
};

// A struct data for storing message queue data.
struct msg_q_data_t {
    int mtype; // For storing message type.
    pid_t pid; // For storing PID info of child process.
    unsigned int io_request; // For storing length of IO request time.
    int is_finished;
};

#endif //PROJ1_COMMON_H
