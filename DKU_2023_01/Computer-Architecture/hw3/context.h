//
// @file : context.c
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that defines context related functions.
//

#ifndef CA_HW1_CONTEXT_H
#define CA_HW1_CONTEXT_H

#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "common.h"

int init_ctx(struct context_t **ctx, int argc, char** argv);
int init_regs(struct context_t *ctx);
int init_mem(struct context_t *ctx);
int print_stats(struct context_t *ctx);
int store_results(struct context_t *ctx);

#endif //CA_HW1_CONTEXT_H
