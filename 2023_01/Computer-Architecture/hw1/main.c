//
// @file : main.c
// @author : Isu Kim @ isu@isu.kim
// @brief : A file that implements functions entry of this program.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "simulator.h"
#include "io.h"
#include "context.h"

struct instruction_t *all_instructions;
struct context_t *ctx;

/**
 * A function that generates instructions and its syntax as well as call back functions.
 */
void generate_instructions() {
    // Generate array of instructions with their respective syntax.
    struct instruction_t instructions[] = {
        { .syntax = {Register, Register, Register}, .token_count = 3, .function = &sim_add, .name = "add"}, // add
        { .syntax = {Register, Register, Register}, .token_count = 3, .function = &sim_sub, .name = "sub"}, // sub
        { .syntax = {Register, Register, Register}, .token_count = 3, .function = &sim_mul, .name = "mul"}, // mul
        { .syntax = {Register, Register, Register}, .token_count = 3, .function = &sim_div, .name = "div"}, // div
        { .syntax = {Register, Both}, .token_count = 2, .function = &sim_mov, .name = "mov"}, // mov
        { .syntax = {Register, Constant}, .token_count = 2, .function = &sim_lw, .name = "lw"}, // lw
        { .syntax = {Register}, .token_count = 1, .function = &sim_sw, .name = "sw"}, // sw
        { .syntax = {}, .token_count = 0, .function = &sim_rst, .name = "rst"}, // rst
        { .syntax = {Constant}, .token_count = 1, .function = &sim_jmp, .name = "jmp"}, // jmp
        { .syntax = {Both, Both, Constant}, .token_count = 3, .function = &sim_beq, .name = "beq"}, // beq
        { .syntax = {Both, Both, Constant}, .token_count = 3, .function = &sim_bne, .name = "bne"}, // bne
        { .syntax = {Register, Both, Both}, .token_count = 3, .function = &sim_slt, .name = "slt"}, // slt
    };

    all_instructions = (struct instruction_t*) malloc(sizeof(struct instruction_t) * INSTRUCTION_COUNT);
    memcpy(all_instructions, instructions, sizeof(struct instruction_t) * INSTRUCTION_COUNT);
}

/**
 * A function that prints out logo with ASCII art.
 * ASCII art from https://patorjk.com/software/taag/ with font ANSI Shadow.
 */
void print_logo() {
    printf("\n███╗   ███╗██╗██████╗ ███████╗██╗███╗   ███╗\n");
    printf("████╗ ████║██║██╔══██╗██╔════╝██║████╗ ████║\n");
    printf("██╔████╔██║██║██████╔╝███████╗██║██╔████╔██║\n");
    printf("██║╚██╔╝██║██║██╔═══╝ ╚════██║██║██║╚██╔╝██║\n");
    printf("██║ ╚═╝ ██║██║██║     ███████║██║██║ ╚═╝ ██║\n");
    printf("╚═╝     ╚═╝╚═╝╚═╝     ╚══════╝╚═╝╚═╝     ╚═╝\n");
    printf("                    A Simple MIPS Calculator\n");
    printf("                            32190984 Isu Kim\n");
}

int main(int argc, char* argv[]) {
    print_logo();
    generate_instructions();

#ifdef DEBUG
    printf("[DEBUG] Successfully generated instructions\n");
    for (uint8_t i = 0 ; i < INSTRUCTION_COUNT ; i++) {
        printf("\t%s: token_count=%d, arg1=%d, arg2=%d, arg3=%d\n", all_instructions[i].name, all_instructions[i].token_count,
               all_instructions[i].syntax[0], all_instructions[i].syntax[1], all_instructions[i].syntax[2]);
    }
#endif
    // Initialize context, parse command-line arguments.
    if (init_ctx(&ctx, argc, argv) == -1) {
        return 1;
    }

    printf("[INFO] Input File: %s\n", ctx->input_file);
    printf("[INFO] Output File: %s\n", ctx->output_file);

#ifdef DEBUG
    printf("[DEBUG] Successfully initialized all context.\n");
#endif

    // Check if input file exists and output file can be opened.
    if (check_file(ctx) == -1) {
        return 1;
    }

    // Load file and check syntax.
    if (load_file(ctx) == -1) {
        printf("[ERROR] Found syntax error, exiting mipsim.\n");
        return 1;
    }

    printf("[SUCCESS] %s has no syntax errors.\n", ctx->input_file);
    printf("[INFO] Starting simulator\n");

    // Perform simulation.
    char err[MAX_STRING] = {0};
    if (execute(ctx, err) == -1) {
        printf("%s\n", err);
        return 1;
    }

    printf("[INFO] mipsim terminated successfully!\n");

    return 0;
}