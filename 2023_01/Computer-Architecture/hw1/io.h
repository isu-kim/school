//
// @file : io.h
// @author : Isu Kim @ isu@isu.kim
// @brief : A file that defines functions for IO operations.
//

#ifndef COMPUTER_ARCH_HW_IO_H
#define COMPUTER_ARCH_HW_IO_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "common.h"
#include "simulator.h"
#include "syntax.h"

int check_file(struct context_t *ctx);
int load_file(struct context_t *ctx);
int write_file(struct context_t ctx, struct user_instruction_t instruction);

#endif //COMPUTER_ARCH_HW_IO_H
