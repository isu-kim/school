//
// @file : utils.h
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that 
//

#ifndef HW2_FINAL_UTILS_H
#define HW2_FINAL_UTILS_H
#pragma once

#include "common.h"

long get_file_size(const char*);
int parse_args(int, char**, char*, int*);
long* split_segments(long, int);
void read_file(char*, long, long);
void set_segments(long, int);
struct so_t* init_shared_objects(int);
struct producer_arg_t* init_producer_args(int, struct so_t*);
struct consumer_arg_t* init_consumer_args(int, struct so_t*, struct word_stat_t*);
struct word_stat_t* init_word_stats(int);
void process_string(char*, struct word_stat_t*);
struct word_stat_t* merge_stats(struct word_stat_t*, int);
void print_stats(struct word_stat_t*);

#endif //HW2_FINAL_UTILS_H
