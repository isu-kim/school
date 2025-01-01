//
// @file : syntax.h
// @author : Isu Kim @ isu@isu.kim
// @brief : A file that defines functions for simple assembly syntax checker.
//

#ifndef COMPUTER_ARCH_HW_SYNTAX_H
#define COMPUTER_ARCH_HW_SYNTAX_H

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>

#include "common.h"

// Macro for to lower string.
#define TO_LOWER(str) \
    do { \
        int len = strlen(str); \
        for (int i = 0; i < len; i++) { \
            str[i] = tolower(str[i]); \
        } \
    } while (0)


int parse_line(const char* line, char *err, struct user_instruction_t *user_instruction);
int parse_param(const char* param, struct param_t *ret, char *err);
uint8_t translate_string(const char *instruction);

#endif //COMPUTER_ARCH_HW_SYNTAX_H
