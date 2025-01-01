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
#include <string.h>

#include "common.h"
#include "utils.h"


int check_file(struct context_t *ctx);
int load_file(struct context_t *ctx);

#endif //COMPUTER_ARCH_HW_IO_H
