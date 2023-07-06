//
// @file : processor.h
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that defines processor related functions.
//


#ifndef CA_HW2_PROCESSOR_H
#define CA_HW2_PROCESSOR_H
#include <string.h>

#include "common.h"
#include "utils.h"

int run(struct context_t *ctx);
int one_cycle(struct context_t *ctx);
int mips_fetch(struct context_t *ctx);
int mips_decode(struct context_t *ctx, struct control_signals_t *signals);
int mips_execute(struct context_t *ctx, struct control_signals_t signals);
int mips_load_store(struct context_t *ctx, struct control_signals_t signals);
int mips_write_back(struct context_t *ctx, struct control_signals_t signals);

int is_finished(struct context_t *ctx);
struct control_signals_t generate_control_signal(uint32_t instruction);
int alu(uint32_t in_1, uint32_t in_2, uint8_t alu_control, uint8_t *zero, uint32_t *alu_result);
int generate_alu_control_signal(uint8_t alu_op_0, uint8_t alu_op_1, uint8_t *alu_control);
void print_used_registers(struct context_t *ctx);
int get_register_name(uint8_t reg_index, char *ret);

#endif //CA_HW2_PROCESSOR_H
