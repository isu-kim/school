//
// @file : context.c
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that implements context related functions.
//

#include "context.h"

extern void print_used_registers(struct context_t *ctx);

/**
 * A function that initializes contexts for current execution.
 * @param ctx The context to store data into.
 * @param argc The argc from cli.
 * @param argv The argv from cli.
 * @return 0 if successful, -1 if failure.
 */
int init_ctx(struct context_t **ctx, int argc, char **argv) {
    // Malloc a new context.
    struct context_t *tmp_ctx = (struct context_t*) malloc(sizeof(struct context_t));
    memset(tmp_ctx, 0, sizeof(struct context_t));

    // Parse arguments using getopt.
    static struct option options[] = {
            {"input", required_argument, 0, 'i'},
            {"output", optional_argument, 0, 'o'},
            {"help", no_argument, 0, 'h'}
    };

    int option_index = 0;
    int opt;
    while ((opt = getopt_long(argc, argv, "i:o:", options, &option_index)) != -1) {
        switch (opt) {
            case 'i': // For input option.
                memset(tmp_ctx->input_file, 0, MAX_STRING);
                strcpy(tmp_ctx->input_file, optarg);
                break;
            case 'h': // For help option.
                printf("Usage: mipsim [options]\n"
                       "Options\n"
                       "  -h, --help           Print this help message and exit\n"
                       "  -i, --input          Specify an input file to execute\n"
                       "  -o, --output         Specify an output file to dump log into\n");
                return -1;
            default: // Unknown options.
            case '?':
                printf("Check --help or -h for more information\n");
                return -1;
        }
    }

    // When input file was not specified.
    if (tmp_ctx->input_file[0] == 0) {
        printf("[ERROR] Input file not specified, use -i option for input file.\n");
        free(tmp_ctx);
        return -1;
    }

    // Initialize registers.
    init_regs(tmp_ctx);

    // Initialize memory with 0.
    if (init_mem(tmp_ctx) != 0) {
        return -1;
    }

    *ctx = tmp_ctx;
    return 0;
}

/**
 * A function that initializes all registers accordingly.
 * @param ctx The current context to apply.
 * @return 0 always.
 */
int init_regs(struct context_t *ctx) {
    // Clear all registers.
    memset(ctx->reg_map, 0, sizeof(uint32_t) * 32);

    // Set initial accordingly to the arguments.
    ctx->reg_map[RA] = 0xFFFFFFFF;
    ctx->reg_map[SP] = 0x1000000;
    ctx->pc = 0x00000000; // Kind of not sure if this is 0x00000000 or 0x10000000.

    return 0;
}

/**
 * A function that initializes memory accordingly.
 * @param ctx The current context to apply.
 * @return -1 if failure, 0 if success.
 */
int init_mem(struct context_t *ctx) {
    // Get 1KB of the binary memory.
    ctx->mem = (uint32_t*) malloc(sizeof(char) * MAX_BIN_SIZE);
    if (!ctx->mem) {
        return -1;
    }

    // Memset with 0.
    memset(ctx->mem, 0, sizeof(char) * MAX_BIN_SIZE);
    return 0;
}

/**
 * A function that prints out the status of total execution.
 * @param ctx The current context.
 * @return -1 if failure, 0 if success.
 */
int print_stats(struct context_t *ctx) {
    printf("-------==[ Stats ]==-------\n");
    printf("- Used Register Values:\n  ");
    print_used_registers(ctx);

    printf("- Total Execution Clocks: %d\n", ctx->clock_count - 1); // We started from 1.
    printf("- Total 'R' Type Instructions: %d\n", ctx->instruction_type_counts[INSTRUCTION_TYPE_R]);
    printf("- Total 'I' Type Instructions: %d\n", ctx->instruction_type_counts[INSTRUCTION_TYPE_I]);
    printf("- Total 'J' Type Instructions: %d\n", ctx->instruction_type_counts[INSTRUCTION_TYPE_J]);
    printf("- Total Branches Taken: %d\n", ctx->branch_taken_count);
    printf("- Total Memory Access (RW): %d\n", ctx->mem_access_count);

    return 0;
}