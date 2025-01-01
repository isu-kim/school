//
// @file : io.c
// @author : Isu Kim @ isu@isu.kim
// @brief : A file that implements functions for IO operations.
//

#include "io.h"

/**
 * A function that checks if input file exists and can be opened.
 * @param ctx The current context.
 * @return 0 if successful, -1 if not.
 */
int check_file(struct context_t *ctx) {
    ctx->in_fp = fopen(ctx->input_file, "r");

    if (ctx->in_fp == NULL) {
        printf("[ERROR] Could not find file %s\n", ctx->input_file);
        return -1;
    }

    return 0;
}

/**
 * A function that loads input file.
 * @param ctx The current context.
 * @return 0 if there is no error with syntax, -1 if not.
 */
int load_file(struct context_t *ctx) {
    // Open input file and retrieve the file size.
    ctx->in_fp = fopen(ctx->input_file, "rb");
    fseek(ctx->in_fp, 0, SEEK_END);
    ctx->in_size = ftell(ctx->in_fp);

    // Check if the size exceeds the max memory size.
    if (ctx->in_size > MAX_BIN_SIZE) {
        printf("[ERROR] The max size that this program can handle is %d bytes\n", MAX_BIN_SIZE);
        return -1;
    }

    // Read whole file to memory. Using (void)! for ignoring -Wunused-result.
    fseek(ctx->in_fp, 0, SEEK_SET);
    (void)! fread(ctx->mem, sizeof(char) * ctx->in_size, 1, ctx->in_fp);

#ifdef DEBUG
    uint32_t instruction_count = ctx->in_size / 4;
    for (uint32_t i = 0 ; i < instruction_count ; i++) {
        uint32_t tmp_instruction = BIG_TO_LITTLE_32(ctx->mem[i]);
        printf("Instruction (%02x): %08x -> INSN %02x", i * 4, tmp_instruction, GET_OPCODE(tmp_instruction));
        if (GET_OPCODE(tmp_instruction) == 0x00) {
            printf(" FUNC %02x\n", GET_FUNCT(tmp_instruction));
        } else {
            printf("\n");
        }
    }
#endif

    return 0;
}