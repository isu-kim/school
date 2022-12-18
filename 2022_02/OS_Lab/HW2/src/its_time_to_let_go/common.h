//
// @file : common.h
// @author : Isu Kim @ dev.gooday2die@gmail.com | Github @ https://github.com/isu-kim
// @brief : A file that defines all common data types.
//

#ifndef HW2_FINAL_COMMON_H
#define HW2_FINAL_COMMON_H
#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <error.h>
#include <errno.h>
#include <signal.h>

#define MAX_STRING 1024
#define MAX_THREADS 100
#define BLOCK_SIZE 4096

#define MAX_STRING_LENGTH 30
#define ASCII_SIZE	256

struct so_t { // For shared object
    pthread_mutex_t lock;
    pthread_cond_t cond;
    int empty;
    char content[BLOCK_SIZE];
    int size;
};

struct word_stat_t { // For word stat
    int stringStat[MAX_STRING_LENGTH];
    int charStat[ASCII_SIZE];
};

struct producer_arg_t { // For producer arguments
    struct so_t* shared_objects;
    long offset;
    long total_bytes;
    int index;
};

struct consumer_arg_t { // For consumer arguments.
    struct so_t* shared_objects;
    struct word_stat_t* stats;
    int index;
};

#endif //HW2_FINAL_COMMON_H
