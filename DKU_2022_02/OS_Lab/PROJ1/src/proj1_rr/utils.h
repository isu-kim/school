//
// @file : utils.h
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that defines utility functions, such as queue.
//

#ifndef PROJ1_UTILS_H
#define PROJ1_UTILS_H
#pragma once

#include "common.h"

#define QUEUE_READY 0
#define QUEUE_WAIT 1
#define QUEUE_INACTIVE 2
#define QUEUE_TERMINATED 3
#define QUEUE_COUNT 4

extern struct pcb_t* queues[4];

int init_queues();
void destroy_queues();
struct pcb_t* pop_queue(int);
void push_queue(int, struct pcb_t);
void push_queue_head(int, struct pcb_t);
void queue_to_str(int, char*);
void dispatch_next();
void wait_queue_to_str(char*);

#endif //PROJ1_UTILS_H
