//
// @file : parent.h
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that implements functions and values for parent process which acts like a kernel process.
//

#ifndef PROJ1_PARENT_H
#define PROJ1_PARENT_H
#pragma once

#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <errno.h>

#include "common.h"
#include "utils.h"

void parent_timer_handler(int);
void move_inactive_queue(struct pcb_t*);
void move_wait_queue();
void decrement_all_io_time();
void time_quantum_over();
void check_ipc();
void log_status();

_Noreturn void parent();
void exit_handler(int);
void print_stats();

#endif //PROJ1_PARENT_H
