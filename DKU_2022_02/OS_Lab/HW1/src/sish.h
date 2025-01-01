//
// @file : sish.h
// @author : Isu Kim @ dev.gooday2die@gmail.com | Github @ https://github.com/gooday2die
// @brief : A file that defines functions for SiSH
//

#ifndef OS_HW1_SISH_H
#define OS_HW1_SISH_H
#pragma once

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>

#include "utils.h"

#define CLEAR_NEW_LINES 100
#define MAX_COMMAND_LEN 300
#define MAX_CWD_LEN 300

int runSiSH(const char**, int);
const char* findWhichBin(const char*, const char**, int);
int processCommand(const char*, const char**, int);
int execute(const char*, char**, pid_t*);
int processDirectBinary(const char**, pid_t*);
int processSpecialCommands(int, const char**);
int executePathBinary(const char**, int, const char**, pid_t*);
void printError(int);
void alrm_handler(int);
void child_handler(int);
#endif //OS_HW1_SISH_H
