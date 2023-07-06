//
// @file : common.h
// @author : Isu Kim @ isu@isu.kim
// @brief : A file that defines commonly used enums and structs.
//

#ifndef COMPUTER_ARCH_HW_COMMON_H
#define COMPUTER_ARCH_HW_COMMON_H
#pragma once

#define MAX_TOKEN_COUNT 3
#define MAX_NAME_LEN 3
#define INSTRUCTION_COUNT 12
#define REGISTER_COUNT 10
#define MAX_STRING 1024
#define MAX_INSTRUCTION_COUNT 1024
#define MAX_ARG_STRING 32
#define UNKNOWN 0xFF

#include <stdint.h>
#include <stdio.h>

// An enum data type for tokens.
enum TokenType {
    Register,
    Constant,
    Both, // This is for cases which we can use register or constant.
};

// An enum data type for instructions.
enum Instruction {
    Add,
    Sub,
    Mul,
    Div,
    Mov,
    Lw,
    Sw,
    Rst,
    Jmp,
    Beq,
    Bne,
    Slt,
};

// A struct that stores parameter value.
struct param_t {
    uint32_t value;
    uint8_t type;
};

// A struct data type for storing user instruction from file.
struct user_instruction_t {
    uint8_t instruction;
    struct param_t param[3];
    uint8_t param_count;
};

// A struct data type for storing current context.
struct context_t {
    char input_file[MAX_STRING];
    char output_file[MAX_STRING];
    FILE *in_fp;
    FILE *out_fp;
    uint32_t pc;
    uint32_t registers[REGISTER_COUNT];
    uint16_t used_map;
    uint32_t last_instruction;
    struct user_instruction_t user_instructions[MAX_INSTRUCTION_COUNT];
};

// A struct data type for storing instruction.
struct instruction_t {
    enum TokenType syntax[MAX_TOKEN_COUNT];
    uint8_t token_count;
    int (*function)(struct context_t *ctx, struct user_instruction_t instruction);
    char name[MAX_NAME_LEN];
};


#endif //COMPUTER_ARCH_HW_COMMON_H
