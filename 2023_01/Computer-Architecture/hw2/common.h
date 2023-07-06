//
// @file : common.h
// @author : Isu Kim @ isu@isu.kim
// @brief : A file that defines commonly used enums and structs.
//

#ifndef COMPUTER_ARCH_HW_COMMON_H
#define COMPUTER_ARCH_HW_COMMON_H
#pragma once

#define REGISTER_COUNT 32
#define MAX_STRING 1024
#define MAX_BIN_SIZE 0x1000000 // Max binary size is 0x1000000 (16M)

// For instruction types.
#define INSTRUCTION_TYPE_R 0x00
#define INSTRUCTION_TYPE_I 0x01
#define INSTRUCTION_TYPE_J 0x02

// Define known registers.
#define ZERO 0
#define AT 1
#define GP 28
#define SP 29
#define FP 30
#define RA 31

#include <stdint.h>
#include <stdio.h>


// Store current execution context.
struct context_t {
    // For execution context.
    uint32_t reg_map[REGISTER_COUNT];
    uint32_t pc;
    uint32_t used_map;
    uint32_t *mem; // The memory data
    // For storing stats on execution.
    uint32_t clock_count;
    uint32_t instruction_type_counts[3];
    uint32_t branch_taken_count;
    uint32_t mem_access_count;
    // For file storing context.
    char input_file[MAX_STRING];
    FILE *in_fp;
    size_t in_size;
    uint8_t reg_read_register_1;
    uint8_t reg_read_register_2;
    uint32_t reg_extended_immediate;
    uint32_t read_data_1;
    uint32_t read_data_2;
    uint32_t alu_result;
    uint32_t mem_read_data;
};

// Store control signals.
struct control_signals_t {
    uint8_t reg_dst;
    uint8_t alu_src;
    uint8_t mem_to_reg;
    uint8_t reg_write;
    uint8_t mem_read;
    uint8_t mem_write;
    uint8_t branch;
    uint8_t jump;
    uint8_t alu_op_0;
    uint8_t alu_op_1;
};

#endif //COMPUTER_ARCH_HW_COMMON_H
