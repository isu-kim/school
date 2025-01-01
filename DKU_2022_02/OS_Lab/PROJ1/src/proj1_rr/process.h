//
// @file : process.h
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that defines functions for process initialization.
//

#ifndef PROJ1_PROCESS_H
#define PROJ1_PROCESS_H
#pragma once

#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

#include "common.h"
#include "child.h"
#include "parent.h"

void run();
void start_processes();

#endif //PROJ1_PROCESS_H
