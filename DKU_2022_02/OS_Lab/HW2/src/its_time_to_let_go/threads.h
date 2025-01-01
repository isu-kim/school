//
// @file : threads.h
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that 
//

#ifndef HW2_FINAL_THREADS_H
#define HW2_FINAL_THREADS_H
#pragma once

#include "common.h"
#include "utils.h"

void* consumer_worker(void*);
void* producer_worker(void*);

#endif //HW2_FINAL_THREADS_H
