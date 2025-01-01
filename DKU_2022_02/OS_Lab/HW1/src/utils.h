//
// @file : utils.h
// @author : Isu Kim @ dev.gooday2die@gmail.com | Github @ https://github.com/gooday2die
// @brief : A file that defines utility functions.
//

#ifndef OS_HW1_UTILS_H
#define OS_HW1_UTILS_H
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 100 // For max buffer size for all utility functions.

const char** getPaths(int*);
const char** parseCommand(int*, const char*);
void printBanner();
void printBye();
void printStringArr_cc(const char**); // Wish overloading was possible :(
void printStringArr_c(char**);

#endif //OS_HW1_UTILS_H
