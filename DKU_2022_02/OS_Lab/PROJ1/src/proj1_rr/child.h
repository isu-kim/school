//
// @file : child.h
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that defines functions and values for child process which acts like a user process.
//

#ifndef PROJ1_CHILD_H
#define PROJ1_CHILD_H
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

#define RANDOM_MAX 20
#define CHILD_MAX_EXECUTION_TIME 50

void child_signal_handler(int);
void child_send_msg(void);
void child_timer_handler(int);
void child();

#endif //PROJ1_CHILD_H
