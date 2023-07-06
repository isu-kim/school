//
// @file : context.c
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that implements context related functions.
//

#include "context.h"

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
            case 'o': // For output option.
                memset(tmp_ctx->output_file, 0, MAX_STRING);
                strcpy(tmp_ctx->output_file, optarg);
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

    // Set default output file if not specified.
    if (tmp_ctx->output_file[0] == 0) {
        strcpy(tmp_ctx->output_file, "out.txt");
        printf("[INFO] Output file not specified, defaults to out.txt\n");
    }

    // Set pc.
    tmp_ctx->pc = 1;
    *ctx = tmp_ctx;
    return 0;
}