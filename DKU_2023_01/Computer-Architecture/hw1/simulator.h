//
// @file : simulator.h
// @author : Isu Kim @ isu@isu.kim
// @brief : A file that defines functions for simulator.
//

#ifndef COMPUTER_ARCH_HW_SIMULATOR_H
#define COMPUTER_ARCH_HW_SIMULATOR_H

#include <string.h>
#include "common.h"
#include "simulator.h"

int sim_add(struct context_t *ctx, struct user_instruction_t instruction);
int sim_sub(struct context_t *ctx, struct user_instruction_t instruction);
int sim_mul(struct context_t *ctx, struct user_instruction_t instruction);
int sim_div(struct context_t *ctx, struct user_instruction_t instruction);
int sim_mov(struct context_t *ctx, struct user_instruction_t instruction);
int sim_lw(struct context_t *ctx, struct user_instruction_t instruction);
int sim_sw(struct context_t *ctx, struct user_instruction_t instruction);
int sim_rst(struct context_t *ctx, __attribute__((unused)) struct user_instruction_t instruction);
int sim_bne(struct context_t *ctx, struct user_instruction_t instruction);
int sim_beq(struct context_t *ctx, struct user_instruction_t instruction);
int sim_slt(struct context_t *ctx, struct user_instruction_t instruction);
int sim_jmp(struct context_t *ctx, struct user_instruction_t instruction);
int mark_registers(struct context_t *ctx, struct user_instruction_t instruction);
int is_used_register(struct context_t ctx, uint16_t r_id);
void print_register_states(struct context_t ctx);
void print_instruction(struct user_instruction_t instruction);
int execute(struct context_t *ctx, char *err);

#endif //COMPUTER_ARCH_HW_SIMULATOR_H
