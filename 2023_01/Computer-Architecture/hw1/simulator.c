//
// @file : simulator.c
// @author : Isu Kim @ isu@isu.kim
// @brief : A file that implements functions for simulator.
//

#include "io.h"

extern struct instruction_t *all_instructions;

/**
 * A function for simulating add instruction.
 * @param ctx The context that is being used.
 * @param instruction The instruction to execute.
 * @return 0, there is no error detection for this function.
 */
int sim_add(struct context_t *ctx, struct user_instruction_t instruction) {
    uint32_t src1 = ctx->registers[instruction.param[1].value];
    uint32_t src2 = ctx->registers[instruction.param[2].value];
    uint32_t *dst = &ctx->registers[instruction.param[0].value];
    *dst = src1 + src2;

    mark_registers(ctx, instruction);
    return 0;
}

/**
 * A function for simulating sub instruction.
 * @param ctx The context that is being used.
 * @param instruction The instruction to execute.
 * @return 0, there is no error detection for this function.
 */
int sim_sub(struct context_t *ctx, struct user_instruction_t instruction) {
    uint32_t src1 = ctx->registers[instruction.param[1].value];
    uint32_t src2 = ctx->registers[instruction.param[2].value];
    uint32_t *dst = &ctx->registers[instruction.param[0].value];
    *dst = src1 - src2;

    mark_registers(ctx, instruction);
    return 0;
}

/**
 * A function for simulating mul instruction.
 * @param ctx The context that is being used.
 * @param instruction The instruction to execute.
 * @return 0, there is no error detection for this function.
 */
int sim_mul(struct context_t *ctx, struct user_instruction_t instruction) {
    uint32_t src1 = ctx->registers[instruction.param[1].value];
    uint32_t src2 = ctx->registers[instruction.param[2].value];
    uint32_t *dst = &ctx->registers[instruction.param[0].value];
    *dst = src1 * src2;

    mark_registers(ctx, instruction);
    return 0;
}

/**
 * A function for simulating div instruction.
 * @param ctx The context that is being used.
 * @param instruction The instruction to execute.
 * @return 0, there is no error detection for this function.
 */
int sim_div(struct context_t *ctx, struct user_instruction_t instruction) {
    uint32_t src1 = ctx->registers[instruction.param[1].value];
    uint32_t src2 = ctx->registers[instruction.param[2].value];
    uint32_t *dst = &ctx->registers[instruction.param[0].value];
    *dst = src1 / src2;

    mark_registers(ctx, instruction);
    return 0;
}

/**
 * A function for simulating mov instruction.
 * @param ctx The context that is being used.
 * @param instruction The instruction to execute.
 * @return 0, there is no error detection for this function.
 */
int sim_mov(struct context_t *ctx, struct user_instruction_t instruction) {
    uint32_t src = ctx->registers[instruction.param[1].value];
    uint32_t *dst = &ctx->registers[instruction.param[0].value];
    *dst = src;

    mark_registers(ctx, instruction);
    return 0;
}

/**
 * A function for simulating lw instruction.
 * @param ctx The context that is being used.
 * @param instruction The instruction to execute.
 * @return 0, there is no error detection for this function.
 */
int sim_lw(struct context_t *ctx, struct user_instruction_t instruction) {
    uint32_t constant = instruction.param[1].value;
    uint32_t *dst = &ctx->registers[instruction.param[0].value];
    *dst =  constant;

    mark_registers(ctx, instruction);
    return 0;
}

/**
 * A function for simulating sw instruction.
 * @param ctx The context that is being used.
 * @param instruction The instruction to execute.
 * @return 0, there is no error detection for this function.
 */
int sim_sw(struct context_t *ctx, struct user_instruction_t instruction) {
    uint32_t src = ctx->registers[instruction.param[0].value];
    printf("\t>> STDOUT: %d\n", src); // SW [SRC] STDOUT prints out the register value.

    mark_registers(ctx, instruction);
    return 0;
}

/**
 * A function for simulating rst instruction.
 * @param ctx The context that is being used.
 * @param instruction The instruction to execute.
 * @return 0, there is no error detection for this function.
 */
int sim_rst(struct context_t *ctx, __attribute__((unused)) struct user_instruction_t instruction) {
    for (uint8_t i = 0 ; i < REGISTER_COUNT ; i++) {
        ctx->registers[i] = 0;
    }
    ctx->used_map = 0x0000; // Reset all usages.

    return 0;
}

/**
 * A function for simulating bne instruction.
 * @param ctx The context that is being used.
 * @param instruction The instruction to execute.
 * @return 0, there is no error detection for this function.
 */
int sim_bne(struct context_t *ctx, struct user_instruction_t instruction) {
    uint8_t val0, val1;
    uint32_t target_pc = instruction.param[2].value;

    // Arguments can become constants or registers. Perform accordingly.
    if (instruction.param[0].type == Register) {
        val0 = ctx->registers[instruction.param[0].value];
    } else {
        val0 = instruction.param[0].value;
    }

    if (instruction.param[1].type == Register) {
        val1 = ctx->registers[instruction.param[1].value];
    } else {
        val1 = instruction.param[1].value;
    }

    // If BNE, jump to target pc.
    // This function will not check if the target pc is valid or not.
    if (val0 != val1) {
        ctx->pc = target_pc;
    }

    return 0;
}

/**
 * A function for simulating beq instruction.
 * @param ctx The context that is being used.
 * @param instruction The instruction to execute.
 * @return 0, there is no error detection for this function.
 */
int sim_beq(struct context_t *ctx, struct user_instruction_t instruction) {
    uint8_t val0, val1;
    uint32_t target_pc = instruction.param[2].value;

    // Arguments can become constants or registers. Perform accordingly.
    if (instruction.param[0].type == Register) {
        val0 = ctx->registers[instruction.param[0].value];
    } else {
        val0 = instruction.param[0].value;
    }

    if (instruction.param[1].type == Register) {
        val1 = ctx->registers[instruction.param[1].value];
    } else {
        val1 = instruction.param[1].value;
    }

    // If BEQ, jump to target pc.
    // This function will not check if the target pc is valid or not.
    if (val0 == val1) {
        ctx->pc = target_pc;
    }

    return 0;
}

/**
 * A function for simulating slt instruction.
 * @param ctx The context that is being used.
 * @param instruction The instruction to execute.
 * @return 0, there is no error detection for this function.
 */
int sim_slt(struct context_t *ctx, struct user_instruction_t instruction) {
    uint8_t val0, val1;
    uint32_t *dst = &ctx->registers[instruction.param[0].value];

    // Arguments can become constants or registers. Perform accordingly.
    if (instruction.param[1].type == Register) {
        val0 = ctx->registers[instruction.param[1].value];
    } else {
        val0 = instruction.param[1].value;
    }

    if (instruction.param[2].type == Register) {
        val1 = ctx->registers[instruction.param[2].value];
    } else {
        val1 = instruction.param[2].value;
    }

    // Perform SLT operation.
    *dst = val0 < val1;

    mark_registers(ctx, instruction);
    return 0;
}

/**
 * A function for simulating jmp instruction.
 * @param ctx The context that is being used.
 * @param instruction The instruction to execute.
 * @return 0, there is no error detection for this function.
 */
int sim_jmp(struct context_t *ctx, struct user_instruction_t instruction) {
    uint32_t target_pc = instruction.param[0].value;

    // This function will not check if the target pc is valid or not.
    ctx->pc = target_pc;

    return 0;
}

/**
 * A function for marking a register as used.
 * @param ctx The content that is being used.
 * @param instruction The instruction for setting argument information.
 * @return -1 if failure, 0 if successful.
 */
int mark_registers(struct context_t *ctx, struct user_instruction_t instruction) {
    for (uint8_t i = 0 ; i < instruction.param_count ; i++) {
        if (instruction.param[i].type == Register) {
            uint16_t new = 1 << instruction.param[i].value;
            ctx->used_map = ctx->used_map | new;
            if (new == 0) return -1; // Used map overflowed.
        }
    }
    return 0;
}

/**
 * A function for checking if a register was used or not.
 * @param ctx The context that is being used.
 * @param r_id The register's id.
 * @return 1 if true, 0 if false.
 */
int is_used_register(struct context_t ctx, uint16_t r_id) {
    return ((ctx.used_map >> r_id) & 0x1) == 1;
}

/**
 * A function for printing out used registers.
 * This is O(n) operation. Shall be optimized in the future.
 */
void print_register_states(struct context_t ctx) {
    for (uint8_t i = 0 ; i < REGISTER_COUNT ; i++) {
        if (is_used_register(ctx, i)) {
            printf("r%d: 0x%x, ", i, ctx.registers[i]);
        }
    }
}

/**
 * A function that prints out instruction information as string.
 * @param instruction The instruction to print out.
 */
void print_instruction(struct user_instruction_t instruction) {
    printf("Instruction : %s ", all_instructions[instruction.instruction].name);

    // Print arguments of the instruction.
    for (uint8_t i = 0 ; i < instruction.param_count ; i++) {
        if (instruction.param[i].type == Register) {
            printf(" r%d ", instruction.param[i].value);
        } else {
            printf(" 0x%x ", instruction.param[i].value);
        }
    }
    printf("\n");
}

/**
 * A function that executes simulator.
 * @param ctx The context to use for simulation.
 * @param err The error string to set if there is one.
 * @return 0 if success, -1 if any error was thrown.
 */
int execute(struct context_t *ctx, char *err) {
    while (ctx->pc != ctx->last_instruction + 1) {
        struct user_instruction_t instruction = ctx->user_instructions[ctx->pc];
        printf("[PC %d] ", ctx->pc);
        print_instruction(instruction);
        ctx->pc++; // Increment pc before executing.
        int ret = all_instructions[instruction.instruction].function(ctx, instruction);

        // When instruction failed.
        if (ret != 0) {
            sprintf(err, "Could not execute instruction: %s @ln%d\n",
                   all_instructions[instruction.instruction].name, ctx->pc);
            return -1;
        }

        // When user requested invalid pc jump.
        if (ctx->pc > ctx->last_instruction) {
            sprintf(err, "Invalid addressing while executing instruction @ ln%d\n",
                    ctx->pc);
            return -1;
        }

        // Otherwise, print instruction information and register status.
        printf("\tRegister Status: ");
        print_register_states(*ctx);
        printf("\n");
        write_file(*ctx, instruction);
    }

    return 0;
}