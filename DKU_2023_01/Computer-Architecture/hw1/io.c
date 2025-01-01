//
// @file : io.c
// @author : Isu Kim @ isu@isu.kim
// @brief : A file that implements functions for IO operations.
//

#include "io.h"

extern struct instruction_t *all_instructions;

/**
 * A function that checks if input file exists and output file can be opened.
 * @param ctx The current context.
 * @return 0 if successful, -1 if not.
 */
int check_file(struct context_t *ctx) {
    ctx->in_fp = fopen(ctx->input_file, "r");
    ctx->out_fp = fopen(ctx->output_file, "w+");

    if (ctx->in_fp == NULL) {
        printf("[ERROR] Could not find file %s\n", ctx->input_file);
        return -1;
    }

    if (ctx->out_fp == NULL) {
        printf("[ERROR] Could not open file %s as output file\n", ctx->output_file);
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
    char buf[MAX_STRING];
    char err[MAX_STRING];
    uint32_t cur_line = 1; // Line starts from 1.
    uint8_t has_error = 0;

    // Iterate over lines and check syntax.
    while (fgets(buf, sizeof(buf), ctx->in_fp) != NULL) {
#ifdef DEBUG
    printf("[DEBUG] Splits: \n");
#endif
        buf[strcspn(buf, "\n")] = '\0'; // Remove new line.
        // Check if syntax is OK.
        struct user_instruction_t parsed;
        int ret = parse_line(buf, err, &parsed);
        if (ret != 0) {
            if (ret == -1) {
                printf(" - Syntax Error @ ln%d \n\t%s\n", cur_line, err);
                has_error = 1;
            } else if (ret == -2) {
                printf(" - Syntax Warning @ ln%d \n\t%s\n", cur_line, err);
            }
            cur_line++;
            continue;
        }

        // If everything is OK, store it to instruction array.
        ctx->user_instructions[cur_line] = parsed;
        cur_line++;
#ifdef DEBUG
        printf("\n");
#endif
    }

    ctx->last_instruction = cur_line - 1;
    return has_error ? -1 : 0;
}

/**
 * A function that writes out the execution result to output file.
 * @param ctx The context that is being used.
 * @param instruction The user's instruction that is being executed right at the moment.
 * @return -1 if failure, 0 if success.
 */
int write_file(struct context_t ctx, struct user_instruction_t instruction) {
    // Write arguments of the instruction.
    fprintf(ctx.out_fp, "Instruction : %s ", all_instructions[instruction.instruction].name);
    for (uint8_t i = 0 ; i < instruction.param_count ; i++) {
        if (instruction.param[i].type == Register) {
            fprintf(ctx.out_fp, " r%d ", instruction.param[i].value);
        } else {
            fprintf(ctx.out_fp, " 0x%x ", instruction.param[i].value);
        }
    }

    fprintf(ctx.out_fp, "\n\tRegisters: ");

    // Write register information
    for (uint8_t i = 0 ; i < REGISTER_COUNT ; i++) {
        if (is_used_register(ctx, i)) {
            fprintf(ctx.out_fp, "r%d: 0x%x, ", i, ctx.registers[i]);
        }
    }

    fprintf(ctx.out_fp, "\n");

    return 0;
}
