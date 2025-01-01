//
// @file : main.c
// @author : Isu Kim @ isu@isu.kim
// @brief : A file that implements functions entry of this program.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "io.h"
#include "context.h"
#include "processor.h"

struct context_t *ctx;

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
    printf("A Bit more complex Single Cycle MIPS Simulator\n");
    printf("                            32190984 Isu Kim\n");
}

int main(int argc, char* argv[]) {
    print_logo();

    // Initialize context, parse command-line arguments.
    if (init_ctx(&ctx, argc, argv) == -1) {
        return 1;
    }

    printf("[INFO] Input File: %s\n", ctx->input_file);

#ifdef DEBUG
    printf("[DEBUG] Successfully initialized all context.\n");
#endif

    // Check if input file exists and output file can be opened.
    if (check_file(ctx) == -1) {
        return 1;
    }

    // Load file and check syntax.
    if (load_file(ctx) == -1) {
        printf("[ERROR] Could not load file, exiting mipsim.\n");
        return 1;
    }

    printf("[INFO] Starting simulator\n");

    // Run the instructions.
    if (run(ctx) == -1) {
        printf("[ERROR] Could not execute instructions.\n");
    }

    printf("[INFO] Simulation terminated successfully!\n");

    // Show stats of execution.
    if (print_stats(ctx) == -1) {
        printf("[ERROR] Could not retrieve stats from execution.\n");
    }

    // Terminate this process.
    printf("[INFO] MIPSim was terminated successfully! Good bye\n");
    return 0;
}
