//
// @file : new_processor.h
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that defines multi-cycle pipelined processor related functions.
//

#ifndef HW3_NEW_PROCESSOR_H
#define HW3_NEW_PROCESSOR_H

#include <stdio.h>
#include <string.h>

#include "common.h"
#include "utils.h"

int new_mips_run(struct context_t *ctx);
int new_mips_one_cycle(struct context_t *ctx);
int new_mips_peek_after(struct context_t *ctx);

int new_mips_instruction_fetch(struct context_t *ctx);
int new_mips_instruction_decode(struct context_t *ctx);
int new_mips_execute(struct context_t *ctx);
int new_mips_mem(struct context_t *ctx);
int new_mips_wb(struct context_t *ctx);
int new_mips_hidden_wb(struct context_t *ctx);

int new_mips_detect_hazard(struct context_t *ctx);
int new_mips_detect_data_hazard(struct context_t *ctx);
int new_mips_stall(struct context_t *ctx);
int new_mips_forward(struct context_t *ctx);
int new_mips_flush(struct context_t *ctx, uint8_t stage);

int new_mips_branch_prediction(struct context_t *ctx, uint8_t is_bubbled);
int new_mips_push_last_branch_prediction(struct context_t *ctx, uint8_t value);
int new_mips_pop_last_branch_prediction(struct context_t *ctx);
int new_mips_validate_branch_prediction(struct context_t *ctx, uint8_t branch_result, uint32_t real_address);

struct control_signals_t generate_control_signal(uint32_t instruction);
int alu(uint32_t in_1, uint32_t in_2, uint8_t alu_control, uint8_t *zero, uint32_t *alu_result, uint32_t instruction);
int generate_alu_control_signal(uint8_t alu_op_0, uint8_t alu_op_1, uint8_t *alu_control, uint32_t instruction);
void print_used_registers(struct context_t *ctx);
int get_register_name(uint8_t reg_index, char *ret);

#endif //HW3_NEW_PROCESSOR_H
