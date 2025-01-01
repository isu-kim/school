//
// @file : utils.h
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that defines utility functions, such as queue.
//

#ifndef PROJ1_UTILS_H
#define PROJ1_UTILS_H
#pragma once

#include "common.h"

#define QUEUE_WAIT 0
#define QUEUE_TERMINATED 1

extern struct pcb_t** queues;
//extern struct workload_t workload;
extern int queue_count;

int init_queues();
void destroy_queues();
struct pcb_t* pop_queue(int);
void push_queue(int, struct pcb_t);
void push_queue_head(int, struct pcb_t);
void queue_to_str(int, char*);
void dispatch_next();
void wait_queue_to_str(char*);
//int load_workload(char*);
int get_queue_size(int);

#endif //PROJ1_UTILS_H
