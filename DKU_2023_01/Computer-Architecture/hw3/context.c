//
// @file : context.c
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that implements context related functions.
//

#include "context.h"

extern void print_used_registers(struct context_t *ctx);

/**
 * A function that initializes contexts for current execution.
 * If there are multiple flags set for branch prediction, the most complex one will be selected.
 * Ex) --bp_static and --bp_2_bit was set, then 2 bit branch prediction will be triggered.
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
            {"help", no_argument, 0, 'h'},
            {"detect_forward", no_argument, 0, 'f'},
            {"bp_static", no_argument, 0, 's'},
            {"bp_1bit", no_argument, 0, '1'},
            {"bp_2bit", no_argument, 0, '2'},
    };

    int option_index = 0;
    int opt;
    while ((opt = getopt_long(argc, argv, "i:o:fs12", options, &option_index)) != -1) {
        switch (opt) {
            case 'i': // For input option.
                memset(tmp_ctx->input_file, 0, MAX_STRING);
                strcpy(tmp_ctx->input_file, optarg);
                break;
            case 'o': // For output option.
                memset(tmp_ctx->output_file, 0, MAX_STRING);
                strcpy(tmp_ctx->output_file, optarg);
                break;
            case 'h': // For help option.
                printf("Usage: mipsim [options]\n"
                       "Options\n"
                       "  -h, --help           Print this help message and exit\n"
                       "  -i, --input          Specify an input file to execute\n"
                       "  -f, --detect_forward Detecting forward and bypass\n"
                       "  -s, --bp_static      For Static Branch prediction\n"
                       "  -1, --bp_1bit        For 1 bit Dynamic Branch prediction\n"
                       "  -2, --bp_2bit        For 2 bit Dynamic prediction\n"
                       "  -o  --output         For storing out the final results\n"
                );
                return -1;
            case 'f': // For detecting forward and bypass.
                tmp_ctx->detect_forward = 1;
                break;
            case 's': // For static branch prediction mode.
                tmp_ctx->bp_mode = BRANCH_MODE_STATIC;
                break;
            case '1': // For 1 bit dynamic branch prediction.
                tmp_ctx->bp_mode = BRANCH_MODE_1_BIT;
                break;
            case '2': // For 2 bit dynamic branch prediction.
                tmp_ctx->bp_mode = BRANCH_MODE_2_BIT;
                break;
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
    printf("- Total Branch Prediction HITs: %d\n", ctx->branch_hit_count);
    printf("- Total Branch Prediction MISSes: %d\n", ctx->branch_miss_count);
    printf("- Total Memory Access (RW): %d\n", ctx->mem_access_count);

    return 0;
}

/**
 * A function that stores register values.
 * This will just store the register id and their values into a file.
 * @param ctx The current context.
 * @return -1 if write failed, 0 if successfuly write.
 */
int store_results(struct context_t *ctx) {
    // No output file was specified. 
    if (strlen(ctx->output_file) == 0) {
        return 0;
    }

    char buf[2048] = {0};
    int count = 0;
    
    for (uint16_t i = 0; i < 32; i++) {
        int length = snprintf(buf + count, sizeof(buf) - count, "%d:0x%08x,", i, ctx->reg_map[i]);
        if (length < 0 || count + length >= sizeof(buf)) {
            // Handle buffer overflow error
            return -1;
        }
        count += length;
    }
    
    // Open the file for writing
    FILE *file = fopen(ctx->output_file, "w");
    if (file == NULL) {
        return -1;
    }
    
    // Write the contents of buf to the file
    fprintf(file, "%s\n", buf);
    
    // Close the file
    fclose(file);
    
    return 0;
}

