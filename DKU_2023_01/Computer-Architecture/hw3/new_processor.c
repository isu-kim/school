//
// @file : new_processor.c
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that implements multi-cycle pipelined processor related functions.
//

#include "new_processor.h"

// A mul hi and lo for mfhi, mflo.
uint32_t mul_hi = 0x00000000;
uint32_t mul_lo = 0x00000000;

/**
 * A function that runs the multi-cycle pipelined processor.
 * @param ctx The current context to run simulation.
 * @return -1 if failure, 0 if terminated successfully.
 */
int new_mips_run(struct context_t *ctx) {
    // Initialize some values.
    ctx->clock_count = 1;
    ctx->last_insn_processed = 0x00;
    ctx->cycles_till_terminate = -1;

    // Until the last instruction which is usually jr $ra hits WB.
    // If we terminate the processor immediately, the instructions before
    // jr $ra will have no chance to perform ID, EX, MEM and WB.
    // This will get us wrong results.
    while (!IS_FINISHED(ctx)) {
        if (new_mips_one_cycle(ctx) == -1) {
            printf("[ERROR] Could not execute cycle: %d\n", ctx->clock_count);
            return -1;
        }
    }

    return 0;
}

/**
 * A function that performs one clock cycle for a pipelined processor.
 * In a real MIPS processor, all stages will be performed at once in one cycle.
 * However, in this program each stages will be executed sequentially.
 * In order to get correct signals for hazard detection, all the pipeline registers
 * and their control signals will be generated before actually performing each stages.
 * This will make our processor to know that in this cycle, we will have some forwarding
 * or some data hazards. Which will help the program execute without wrong signals.
 * @param ctx The current context.
 * @return -1 if failure, 0 if cycle executed without error.
 */
int new_mips_one_cycle(struct context_t *ctx) {
    printf("[PC: %x, CYCLE: %d]\n", ctx->pc, ctx->clock_count);
    print_used_registers(ctx);

    // Generate pipeline register prior to executing stages.
    // As mentioned up in the comment, this will help us deal with
    // detecting hazards.
    int ret = new_mips_peek_after(ctx);
    if (ret != 0) {
        return -1;
    }

    // If forwarding option was turned off, just detect data hazard.
    // In this stage the program will perform stalls for data hazards.
    if (!ctx->detect_forward) {
        ret = new_mips_detect_hazard(ctx);
        if (ret != 0) {
            return -1;
        }
    }

    // In one MIPS clock cycle, the processor will perform WB in the
    // earlier half, then perform ID in the later half.
    // In some cases, a register written in WB is used in ID.
    // If we do not perform WB before ID, this will get us
    // outdated register values. Therefore, perform WB prior to stages.
    ret = new_mips_hidden_wb(ctx);
    if (ret != 0) {
        return -1;
    }

    // A typical IF stage for pipeline processor.
    ret = new_mips_instruction_fetch(ctx);
    if (ret != 0) {
        return -1;
    }

    // A typical ID stage for pipeline processor.
    ret = new_mips_instruction_decode(ctx);
    if (ret != 0) {
        return -1;
    }

    // A typical EX stage for pipeline processor.
    ret = new_mips_execute(ctx);
    if (ret != 0) {
        return -1;
    }

    // A typical MEM stage for pipeline processor.
    ret = new_mips_mem(ctx);
    if (ret != 0) {
        return -1;
    }

    // A typical WB stage for pipeline processor.
    ret = new_mips_wb(ctx);
    if (ret != 0) {
        return -1;
    }

    // If forwarding option was turned on, perform forwarding.
    // The forwarding will be performed in the end of the cycle.
    if (ctx->detect_forward) {
        ret = new_mips_detect_hazard(ctx);
        if (ret != 0) {
            return -1;
        }
    }

    // Perform branch prediction for next cycle's PC.
    if (ctx->need_branch_prediction) {
        // If current ID is bubbled, send the bubbled information as well.
        new_mips_branch_prediction(ctx, (ctx->stall_status & 0x2) != 0);
    }

    // Increment clock count and shift bubbles.
    // cycles_till_terminate will determine if the processor had enough clocks
    // before terminating jr $ra. Meaning that the instructions before had
    // enough clocks for ID, EX, MEM, and WB.
    ctx->clock_count++;
    ctx->stall_status = ctx->stall_status << 1;
    ctx->cycles_till_terminate = ctx->cycles_till_terminate <0 ? -1 : ctx->cycles_till_terminate - 1;

    // The current clock passed. Thus send the tmp pipeline registers
    // to the next clock's pipeline registers.
    // If we just store directly to new pipeline registers, the program will
    // just execute like a single cycle processor since that will be executed
    // sequentially.
    memcpy(&ctx->old_pipeline_reg, &ctx->new_pipeline_reg, sizeof(struct pipeline_register_t));

    return 0;
}

/**
 * A function that peeks the current cycle's pipeline registers.
 * This function is due to the nature of this program having no capacity
 * to execute multiple instruction stages at once.
 * This function will set values for pipeline registers for determining
 * and detecting potential hazards correctly.
 * Yes, this function acts really dumb.
 * Too bad, I have no enough time for better solutions.
 * Yes, I literally wanted to kill myself writing this function.
 * @deprecated
 * @param ctx The current context.
 * @return -1 if failure, 0 if success.
 */
int new_mips_peek_after(struct context_t *ctx) {
    // Retrieve future and old pipeline registers.
    struct pipeline_register_t *future_pr = &ctx->future_pipeline_reg;
    struct pipeline_register_t *old_pr = &ctx->old_pipeline_reg;

    // Set IF/ID pipeline registers.
    uint32_t instruction = 0x00000000;
    if (ctx->pc != 0xFFFFFFFF) {
        instruction = BIG_TO_LITTLE_32(ctx->mem[CONVERT_PC(ctx->pc)]);
    }
    SET_IF_ID_INSN(future_pr, instruction);

    // Set ID/EX pipeline registers.
    uint32_t current_instruction = GET_IF_ID_INSN(old_pr);
    struct control_signals_t signals = generate_control_signal(current_instruction);

    SET_ID_EX_INSN(future_pr, current_instruction);
    SET_ID_EX_CTR_SIG(future_pr, signals);
    SET_ID_EX_RS(future_pr, GET_RS(current_instruction));
    SET_ID_EX_RT(future_pr, GET_RT(current_instruction));
    SET_ID_EX_WRITE_RT(future_pr, GET_RT(current_instruction));
    SET_ID_EX_WRITE_RD(future_pr, GET_RD(current_instruction));
    SET_ID_EX_READ_DATA_1(future_pr, GET_ID_EX_READ_DATA_1(old_pr));
    SET_ID_EX_READ_DATA_2(future_pr, GET_ID_EX_READ_DATA_2(old_pr));
    SET_ID_EX_SIGN_EXTEND(future_pr, (int32_t)(int16_t)GET_IMM(current_instruction));

    signals = GET_ID_EX_CTR_SIG(old_pr);
    uint32_t alu_result, mux_out;
    uint8_t alu_control, zero;
    mux_out = MUX(GET_ID_EX_SIGN_EXTEND(old_pr), GET_ID_EX_READ_DATA_2(old_pr), signals.alu_src);

    // In some cases, right before cycle may change the data in register.
    // So check if the inputs were forwarded before and was changed by wb.
    // If the inputs were changed by wb, apply the change to the inputs.
    // Otherwise, we will be getting some outdated (1 cycle later) data.
    uint32_t id_ex_read_data_1, id_ex_read_data_2;
    id_ex_read_data_1 = ctx->reg_map[GET_ID_EX_RS(old_pr)];
    id_ex_read_data_2 = ctx->reg_map[GET_ID_EX_RT(old_pr)];

    // Set ID/EX.ReadData1, but consider if this was forwarded or not
    // also consider if this was referring to a register that was updated in the same cycle.
    if ((id_ex_read_data_1 != GET_ID_EX_READ_DATA_1(future_pr)) && (ctx->before_forward & 0x01) == 0) {
        SET_ID_EX_READ_DATA_1(future_pr, id_ex_read_data_1);
    }

    if ((id_ex_read_data_2 != GET_ID_EX_READ_DATA_2(future_pr)) && (ctx->before_forward & 0x2) == 0) {
        SET_ID_EX_READ_DATA_2(future_pr, id_ex_read_data_2);
    }

    // Set EX/MEM pipeline registers.
    generate_alu_control_signal(signals.alu_op_0, signals.alu_op_1, &alu_control, GET_ID_EX_INSN(old_pr));
    alu(GET_ID_EX_READ_DATA_1(future_pr), mux_out, alu_control, &zero, &alu_result, GET_ID_EX_INSN(old_pr));
    SET_EX_MEM_ALU_RESULT(future_pr, alu_result);
    SET_EX_MEM_ZERO(future_pr, zero);

    mux_out = MUX(GET_ID_EX_WRITE_RD(old_pr), GET_ID_EX_WRITE_RT(old_pr), signals.reg_dst);
    SET_EX_MEM_WRITE_TARGET(future_pr, mux_out);
    SET_EX_MEM_CTR_SIG(future_pr, signals);

    signals = GET_EX_MEM_CTR_SIG(old_pr);
    SET_MEM_WB_WRITE_TARGET(future_pr, GET_EX_MEM_WRITE_TARGET(old_pr));
    SET_MEM_WB_CTR_SIG(future_pr, signals);

    // Set MEM/WB pipeline registers.

    char* offset = (char *)ctx->mem;
    offset = offset + GET_EX_MEM_ALU_RESULT(old_pr); // Calculate offset in memory.
    if (signals.mem_read) { // If this had MemRead signal turned on.
        uint32_t mem_read_data = 0x00000000;
        // The code below might make segfault since it does not check boundaries.
        // @deprecated
        memcpy(&mem_read_data, offset, 4); // Read data from memory.
        SET_MEM_WB_READ_DATA(future_pr, mem_read_data);
    }
    SET_MEM_WB_ALU_RESULT(future_pr, GET_EX_MEM_ALU_RESULT(old_pr));

    // Generate WB pipeline register.
    // There ain't a WB pipeline register, however we need to have this
    // in order to detect data hazards when we are not using forwarding.
    // Yes, this is dumb
    signals = GET_MEM_WB_CTR_SIG(old_pr);
    SET_WB_WRITE_TARGET(future_pr, GET_MEM_WB_WRITE_TARGET(old_pr));
    SET_WB_SIG_WRITE_REG(future_pr, signals.reg_write);

    return 0;
}

/**
 * A function that detects data hazards.
 * This will automatically select forwarding or just stalling accordingly.
 * @param ctx The current context.
 * @return 0 always.
 */
int new_mips_detect_hazard(struct context_t *ctx) {
    // Detect data hazard.
    if (ctx->detect_forward) {
        // If forwarding option was set, forward if possible.
        new_mips_forward(ctx);
    } else {
        // If forwarding option was not set, just stall.
        if (new_mips_detect_data_hazard(ctx)) {
            new_mips_stall(ctx);
        }
    }
    return 0;
}

/**
 * A function that detects data hazards.
 * This will detect data hazards so that this can be used for non-forwarding solutions.
 * Unfortunately, this will not detect load-use hazard.
 * @param ctx The current context.
 * @return 1 if there is at least 1 data hazard, 0 if not.
 */
int new_mips_detect_data_hazard(struct context_t *ctx) {
    // From the future signals, we detect hazards.
    struct pipeline_register_t *future_pr = &ctx->future_pipeline_reg;

    // Case 1) EX/MEM.target == ID/EX.rs or ID/EX.rd
    if ((GET_EX_MEM_WRITE_TARGET(future_pr) != 0 && GET_EX_MEM_CTR_SIG(future_pr).reg_write == 1)
        && ((GET_EX_MEM_WRITE_TARGET(future_pr) == GET_ID_EX_RS(future_pr))
            || GET_EX_MEM_WRITE_TARGET(future_pr) == GET_ID_EX_RT(future_pr))) {
        return 1;
    }

    // Case 2) MEM/WB.target == ID/EX.rs or ID/EX.rd
    if ((GET_MEM_WB_WRITE_TARGET(future_pr) != 0 && GET_MEM_WB_CTR_SIG(future_pr).reg_write == 1)
        && ((GET_MEM_WB_WRITE_TARGET(future_pr) == GET_ID_EX_RS(future_pr))
            || GET_MEM_WB_WRITE_TARGET(future_pr) == GET_ID_EX_RT(future_pr))) {
        return 1;
    }

    // Case 3) WB.target == ID/EX.rs or ID/EX.rd
    //if ((GET_WB_WRITE_TARGET(future_pr) != 0 && GET_WB_SIG_WRITE_REG(future_pr) == 1)
     ///   && ((GET_WB_WRITE_TARGET(future_pr) == GET_ID_EX_RS(future_pr))
       //     || GET_WB_WRITE_TARGET(future_pr) == GET_ID_EX_RT(future_pr))) {
    //    return 1;
    //`}

    return 0;
}

/**
 * A function that performs stall.
 * This will mark IF and ID as stalled.
 * @param ctx The current context.
 * @return 0 always.
 */
int new_mips_stall(struct context_t *ctx) {
    // Mark IF and ID as stalled.
    ctx->stall_status = ctx->stall_status | 0x03;
    return 0;
}

/**
 * A function that performs IF stage.
 * @param ctx The current context.
 * @return -1 if failure, 0 if success.
 */
int new_mips_instruction_fetch(struct context_t *ctx) {
    printf(" [IF]\n");

    // If last instruction is in pipeline, do not fetch anymore.
    if (((ctx->last_insn_processed & 0x01) != 0) || (ctx->pc == 0xFFFFFFFF)) {
        printf("    [END] No more instructions to fetch.\n");
        return 0;
    }

    // Fetch instruction from memory to CPU.
    uint32_t instruction = BIG_TO_LITTLE_32(ctx->mem[CONVERT_PC(ctx->pc)]);
    printf("    [IM] PC: 0x%08x -> 0x%08x\n", ctx->pc, instruction);

    // If this stage was bubbled, don't do anything further.
    if ((ctx->stall_status & 0x1) != 0) {
        ctx->stall_status = ctx->stall_status & 0xFE;
        return 0;
    }

    // If this stage was not bubbled, increment PC and store
    // pipeline registers with correct values.
    ctx->pc = ctx->pc + 4;
    SET_IF_ID_INSN(&ctx->new_pipeline_reg, instruction);
    SET_IF_ID_PC(&ctx->new_pipeline_reg, ctx->pc + 4);

    // Check if we need branch prediction in next cycle.
    ctx->need_branch_prediction = NEED_BRANCH_PREDICTION(instruction);

    return 0;
}

/**
 * A function that performs ID stage.
 * @param ctx The current context.
 * @return -1 if failure, 0 if success.
 */
int new_mips_instruction_decode(struct context_t *ctx) {
    printf(" [ID]\n");

    // If last instruction is in pipeline, do not fetch anymore.
    if ((ctx->last_insn_processed & 0x02) != 0) {
        printf("    [END] No more instructions to decode.\n");
        return 0;
    }

    // Control signals for current decode stage.
    struct control_signals_t control_signals = {0};
    struct pipeline_register_t *old_pr = &ctx->old_pipeline_reg;
    struct pipeline_register_t *new_pr = &ctx->new_pipeline_reg;
    uint32_t current_instruction = GET_IF_ID_INSN(old_pr);

    // Generate other values for pipeline registers.
    uint8_t reg_read_register_1 = GET_RS(current_instruction);
    uint8_t reg_read_register_2 = GET_RT(current_instruction);
    uint32_t extended_imm = (int32_t)(int16_t)GET_IMM(current_instruction);

    // Set those generated signals.
    SET_ID_EX_PC(new_pr, GET_IF_ID_PC(old_pr));
    SET_ID_EX_INSN(new_pr, current_instruction);
    SET_ID_EX_READ_DATA_1(new_pr, ctx->reg_map[reg_read_register_1]);
    SET_ID_EX_READ_DATA_2(new_pr, ctx->reg_map[reg_read_register_2]);
    SET_ID_EX_SIGN_EXTEND(new_pr, extended_imm);
    SET_ID_EX_WRITE_RT(new_pr, GET_RT(current_instruction));
    SET_ID_EX_WRITE_RD(new_pr, GET_RD(current_instruction));
    SET_ID_EX_JUMP_ADDR(new_pr, GET_ADDR(current_instruction));
    SET_ID_EX_RS(new_pr, GET_RS(current_instruction));
    SET_ID_EX_RT(new_pr, GET_RT(current_instruction));

    // Check the instruction of the jump.
    // This is not how MIPS works, but for implementation.
    // We will also store the jump type information for future use.
    // 1 is for JR, 2 is for JAL, 0 is for anything besides those.
    uint8_t jump_type = GET_FUNCT(current_instruction) == FUNCT_JR ? JUMP_JR :
                        GET_OPCODE(current_instruction) == OPCODE_JAL ? JUMP_JAL : 0;
    SET_ID_EX_JUMP_TYPE(new_pr, jump_type);

    // If this ID was stalled, set control signals as 0.
    if ((ctx->stall_status & 0x02) != 0) {
        printf("    [BUBBLE]\n");
        memset(&control_signals, 0, sizeof(struct control_signals_t));
        SET_ID_EX_CTR_SIG(new_pr, control_signals);
        return 0;
    } else {
        // If not stalled, generate control signals and set it.
        control_signals = generate_control_signal(current_instruction);
        SET_ID_EX_CTR_SIG(new_pr, control_signals);
    }

    // Print out stats.
    char buff[150] = {0};
    uint8_t opcode = GET_OPCODE(current_instruction);
    if (IS_R_TYPE(opcode)) {
        sprintf(buff, "type: R, opcode: 0x%x, rs: 0x%x (R[%d]=0x%x), rt: 0x%x (R[%d]=0x%x), "
                      "rd: 0x%x (%d), shamt: 0x%x, funct: 0x%x",
                GET_OPCODE(current_instruction),
                GET_RS(current_instruction), GET_RS(current_instruction), ctx->reg_map[GET_RS(current_instruction)],
                GET_RT(current_instruction), GET_RT(current_instruction), ctx->reg_map[GET_RT(current_instruction)],
                GET_RD(current_instruction), GET_RD(current_instruction), GET_SHAMT(current_instruction), GET_FUNCT(current_instruction));
        ctx->instruction_type_counts[INSTRUCTION_TYPE_R]++;
    } else if (IS_J_TYPE(opcode)) {
        sprintf(buff, "type: J, address: 0x%x", GET_ADDR(current_instruction));
        ctx->instruction_type_counts[INSTRUCTION_TYPE_J]++;
    } else if (IS_I_TYPE(opcode)) {
        sprintf(buff, "type: I, opcode: 0x%x, rs: 0x%x (R[%d]=0x%x), rt: 0x%x (R[%d]=0x%x), "
                      "imm: 0x%x",
                GET_OPCODE(current_instruction),
                GET_RS(current_instruction), GET_RS(current_instruction), ctx->reg_map[GET_RS(current_instruction)],
                GET_RT(current_instruction), GET_RT(current_instruction), ctx->reg_map[GET_RT(current_instruction)],
                GET_IMM(current_instruction));
        ctx->instruction_type_counts[INSTRUCTION_TYPE_I]++;
    }

    printf("    %s \n", buff);
    return 0;
}

/**
 * A function that performs EX stage.
 * @param ctx The current context.
 * @return -1 if failure, 0 if success.
 */
int new_mips_execute(struct context_t *ctx) {
    printf(" [EX]\n");

    // If last instruction is in pipeline, do not fetch anymore.
    if ((ctx->last_insn_processed & 0x04) != 0) {
        printf("    [END] No more instructions to execute.\n");
        return 0;
    }

    // Retrieve pipeline register and control signals.
    struct pipeline_register_t *new_pr = &ctx->new_pipeline_reg;
    struct pipeline_register_t *old_pr = &ctx->old_pipeline_reg;
    struct control_signals_t control_signals = GET_ID_EX_CTR_SIG(old_pr);


#ifdef DEBUG
    printf("    [DEBUG] ID/EX.PC: %x, ID/EX.INSN %x, ID/EX.READ_DATA_1 %x, ID/EX.READ_DATA_2 %x, ID/EX.SIGN_EXTEND %x, ID/EX.JUMP_ADDR %x, "
           "ID/EX.WRITE_RT: %x, ID/EX.WRITE_RD: %x, ID/EX.JUMP_TYPE: %x\n", GET_ID_EX_INSN(old_pr), GET_ID_EX_INSN(old_pr),
           GET_ID_EX_READ_DATA_1(old_pr), GET_ID_EX_READ_DATA_2(old_pr), GET_ID_EX_SIGN_EXTEND(old_pr),
           GET_ID_EX_JUMP_ADDR(old_pr), GET_ID_EX_WRITE_RT(old_pr), GET_ID_EX_WRITE_RD(old_pr), GET_ID_EX_JUMP_TYPE(old_pr));
    DEBUG_PRINT_SIGNALS(&GET_ID_EX_CTR_SIG(old_pr));
#endif

    // Decide ALUSrc using MUX from ID/EX pipeline register.
    uint32_t mux_out = MUX(GET_ID_EX_SIGN_EXTEND(old_pr), GET_ID_EX_READ_DATA_2(old_pr), control_signals.alu_src);

    // Generate ALU control signal from ALU control based upon the diagram.
    uint8_t alu_control;
    int ret = generate_alu_control_signal(control_signals.alu_op_0, control_signals.alu_op_1, &alu_control, GET_ID_EX_INSN(old_pr));
    if (ret != 0) { // Something went terribly wrong.
        printf("[ERROR] Could not generate ALU control signal at 0x%08x\n", GET_ID_EX_PC(new_pr));
        return -1;
    }

    // Get return values from the ALU, then store result and zero to pipeline register.
    uint8_t zero = 0;
    uint32_t alu_result = 0x00000000;
    ret = alu(GET_ID_EX_READ_DATA_1(old_pr), mux_out, alu_control, &zero, &alu_result, GET_ID_EX_INSN(old_pr));

    SET_EX_MEM_ALU_RESULT(new_pr, alu_result);
    SET_EX_MEM_ZERO(new_pr, zero);
    if (ret != 0) {
        printf("[ERROR] Could not execute instruction in ALU at %x\n", GET_ID_EX_PC(old_pr));
        return -1;
    }

    // Store ID/EX.ReadData2 to the next stage.
    SET_EX_MEM_READ_DATA_2(new_pr, GET_ID_EX_READ_DATA_2(old_pr));

#ifdef DEBUG
    // Print out the results.
    printf("    [DEBUG] EX: ALU Control (0x%x)\n", alu_control);
    printf("   - ALU Result: 0x%08x / Zero %d\n", alu_result, zero);
#endif

    // Calculate branch address.
    uint32_t tmp_pc = GET_ID_EX_PC(old_pr);
    tmp_pc = tmp_pc + (GET_ID_EX_SIGN_EXTEND(old_pr) << 2);
    SET_EX_MEM_PC_ADD_RESULT(new_pr, tmp_pc);

    // Determine the register to write back.
    mux_out = MUX(GET_ID_EX_WRITE_RD(old_pr), GET_ID_EX_WRITE_RT(old_pr), control_signals.reg_dst);
    SET_EX_MEM_WRITE_TARGET(new_pr, mux_out);

    // For jump, forward the jump information.
    SET_EX_MEM_JUMP_ADDR(new_pr, GET_ID_EX_JUMP_ADDR(old_pr));
    SET_EX_MEM_JUMP_TYPE(new_pr, GET_ID_EX_JUMP_TYPE(old_pr));
    SET_EX_MEM_PC(new_pr, GET_ID_EX_PC(old_pr));
    SET_EX_MEM_CTR_SIG(new_pr, control_signals);

    // If this ID was stalled, set control signals as 0.
    if ((ctx->stall_status & 0x04) != 0) {
        printf("    [BUBBLE]\n");
        return 0;
    }

    // Print out the results.
    if (control_signals.alu_src == 0) {
        printf("    rs: %x, rt: %x, alu_result: 0x%08x\n", GET_ID_EX_READ_DATA_1(old_pr),
               GET_ID_EX_READ_DATA_2(old_pr), alu_result);
    } else {
        printf("    rs: %x, imm: %x, alu_result: 0x%08x\n", GET_ID_EX_READ_DATA_1(old_pr),
               GET_ID_EX_SIGN_EXTEND(old_pr), alu_result);
    }

    return 0;
}

/**
 * A function that performs MEM stage.
 * MEM stage will perform branch prediction validation
 * as well as jump and setting the PC into the target address.
 * @param ctx The current context.
 * @return -1 if failure, 0 if success.
 */
int new_mips_mem(struct context_t *ctx) {
    printf(" [MEM]\n");

    // If last instruction is in pipeline, do not fetch anymore.
    if ((ctx->last_insn_processed & 0x08) != 0) {
        printf("    [END] No more instructions to access memory.\n");
        return 0;
    }

    // Retrieve pipeline register and control signals.
    struct pipeline_register_t *new_pr = &ctx->new_pipeline_reg;
    struct pipeline_register_t *old_pr = &ctx->old_pipeline_reg;
    struct control_signals_t control_signals = GET_EX_MEM_CTR_SIG(old_pr);

#ifdef DEBUG
    printf("    [DEBUG] EX/MEM.PC_ADD_RESULT: %x, EX/MEM.ALU_RESULT: %x, EX/MEM.READ_DATA_2: %x, EX/MEM.JUMP_ADDR: %x"
           " EX/MEM.JUMP_TYPE: %x, EX/MEM.WRITE_TARGET: %x, EX/MEM.ZERO: %x\n", GET_EX_MEM_PC_ADD_RESULT(old_pr),
           GET_EX_MEM_ALU_RESULT(old_pr), GET_EX_MEM_READ_DATA_2(old_pr), GET_EX_MEM_JUMP_ADDR(old_pr),
           GET_EX_MEM_JUMP_TYPE(old_pr), GET_EX_MEM_WRITE_TARGET(old_pr), GET_EX_MEM_ZERO(old_pr));
    DEBUG_PRINT_SIGNALS(&GET_EX_MEM_CTR_SIG(old_pr));
#endif

    // Retrieve ALUResult from EX/MEM pipeline register.
    // Check memory boundary if this was valid memory address.
    // This is the last method of protecting this program.
    // Below here is just memcpy without any checks.
    uint32_t alu_result = GET_EX_MEM_ALU_RESULT(old_pr);
    if ((control_signals.mem_read || control_signals.mem_write) && alu_result > MAX_BIN_SIZE) {
        printf("    [ERROR] Given address is invalid: 0x%08x\n", alu_result);
        return -1;
    }

    // Start reading data from the memory.
    char* offset = (char *)ctx->mem;
    void* ret = NULL;
    offset = offset + alu_result; // Calculate offset in memory.

    // If this had MemWrite signal turned on.
    if (control_signals.mem_write) {
        uint32_t read_data_2 = GET_EX_MEM_READ_DATA_2(old_pr);
        ret = memcpy(offset, &read_data_2, 4); // Store data to memory.
        if (!ret) { // memcpy failed.
            return -1;
        }

        printf("    written 0x%08x to 0x%08x\n", read_data_2, alu_result);
        ctx->mem_access_count++;
    }

    // If this had MemRead signal turned on.
    if (control_signals.mem_read) {
        uint32_t mem_read_data = 0x00000000;
        ret = memcpy(&mem_read_data, offset, 4); // Read data from memory.
        if (!ret) { // memcpy failed.
            return -1;
        }

        printf("    read 0x%08x from 0x%08x\n", mem_read_data, alu_result);
        SET_MEM_WB_READ_DATA(new_pr, mem_read_data);
        ctx->mem_access_count++;
    }

    // The MEM stage will actually validate branch prediction.
    // this will check if the last branch prediction was correct or not.
    if (control_signals.branch && !control_signals.jump) {
        uint8_t zero = GET_EX_MEM_ZERO(old_pr);
        uint8_t actual = ((control_signals.branch & zero) & 0x1) == 1; // 1 if taken, 0 if not taken.

        uint32_t real_pc = actual ? GET_EX_MEM_PC_ADD_RESULT(old_pr) : GET_EX_MEM_PC(old_pr);
        new_mips_validate_branch_prediction(ctx, actual, real_pc - 4);
    }

    // The MEM stage will perform jump instructions as well.
    if (control_signals.jump) {
        // Calculate jump target address and jump type.
        uint32_t jump_target = GET_EX_MEM_JUMP_ADDR(old_pr);
        uint32_t jump_type = GET_EX_MEM_JUMP_TYPE(old_pr);

        // Calculate the target address.
        jump_target = jump_target << 2;
        jump_target = jump_target | GET_PC_UPPER(GET_EX_MEM_PC(old_pr));

        // If JR, store ALUResult to PC.
        if (IS_JR(jump_type)) {
            ctx->pc = GET_EX_MEM_ALU_RESULT(old_pr);
        }

        // If JAL, store PC to RA.
        if (IS_JAL(jump_type)) {
            ctx->reg_map[RA] = GET_EX_MEM_PC(old_pr);
            ctx->pc = jump_target;
        }
        printf("    jump taken, PC=0x%08x\n", ctx->pc);

        // Once jump is taken, flush everything before EX stage.
        new_mips_flush(ctx, STAGE_IF);
        new_mips_flush(ctx, STAGE_ID);
        ctx->jump_count++;

        // If the jumped address is 0xFFFFFFFF, after 1 extra clock terminate.
        ctx->cycles_till_terminate = ctx->pc == 0xFFFFFFFF ? 1 : -1;
    }

    // Forward signals to next clock.
    SET_MEM_WB_ALU_RESULT(new_pr, GET_EX_MEM_ALU_RESULT(old_pr));
    SET_MEM_WB_WRITE_TARGET(new_pr, GET_EX_MEM_WRITE_TARGET(old_pr));
    SET_MEM_WB_PC(new_pr, GET_EX_MEM_PC(old_pr));
    SET_MEM_WB_CTR_SIG(new_pr, control_signals);

    // If this ID was stalled, set control signals as 0.
    if ((ctx->stall_status & 0x08) != 0) {
        printf("    [BUBBLE]\n");
	struct control_signals_t clean = {0};
        SET_MEM_WB_CTR_SIG(new_pr, clean);
        return 0;
    }

    return 0;
}

/**
 * A function that performs WB stage.
 * @param ctx The current context.
 * @return -1 if failure, 0 if success.
 */
int new_mips_wb(struct context_t *ctx) {
    printf(" [WB]\n");

    // If last instruction is in pipeline, do not fetch anymore.
    if ((ctx->last_insn_processed & 0x10) != 0) {
        printf("    [END] No more instructions to write back.\n");
        return 0;
    }

    // Retrieve pipeline register and control signals.
    struct pipeline_register_t *tmp_reg = &ctx->new_pipeline_reg;
    struct pipeline_register_t *pipeline_reg = &ctx->old_pipeline_reg;
    const struct control_signals_t signals = GET_MEM_WB_CTR_SIG(pipeline_reg);

    // Determine the data to store, whether this is a data from memory or this is a data from ALU result.
    uint32_t to_store = MUX(GET_MEM_WB_READ_DATA(pipeline_reg), GET_MEM_WB_ALU_RESULT(pipeline_reg), signals.mem_to_reg);

    // Determine where to write register, the target was selected before.
    uint8_t write_register = GET_MEM_WB_WRITE_TARGET(pipeline_reg);
    SET_WB_WRITE_TARGET(tmp_reg, write_register);
    SET_WB_SIG_WRITE_REG(tmp_reg, signals.reg_write);

    // If this stage was bubbled, show it
    if ((ctx->stall_status & 0x10) != 0) {
        printf("    [BUBBLE]\n");
        return 0;
    }

    // If RegWrite, write data to register file.
    if (signals.reg_write) {
        if (write_register > REGISTER_COUNT && to_store < 0) { // If the register was invalid, quit.
            printf("[ERROR] Invalid register index: %d\n", write_register);
            return -1;
        } else { // If valid, store data.
            ctx->reg_map[write_register] = to_store;
            if (write_register != 0) SET_USED_REG(ctx->used_map, write_register);
            printf("    r[%d] <- 0x%08x\n", write_register, to_store);
        }
    }

    return 0;
}

/**
 * A function that performs WB implicitly.
 * Since MIPS stores WB into register file first and then
 * reads register file in ID stage in the later half cycle,
 * this function will act like a WB into register file
 * in the first half cycle.
 * @param ctx The current context.
 * @return -1 if failure, 0 if success.
 */
int new_mips_hidden_wb(struct context_t *ctx) {
    // Retrieve pipeline register and control signals.
    struct pipeline_register_t *tmp_reg = &ctx->new_pipeline_reg;
    struct pipeline_register_t *pipeline_reg = &ctx->old_pipeline_reg;
    const struct control_signals_t signals = GET_MEM_WB_CTR_SIG(pipeline_reg);

    // Determine the data to store, whether this is a data from memory or this is a data from ALU result.
    uint32_t to_store = MUX(GET_MEM_WB_READ_DATA(pipeline_reg), GET_MEM_WB_ALU_RESULT(pipeline_reg), signals.mem_to_reg);

    // Determine where to write register, the target was selected before.
    uint8_t write_register = GET_MEM_WB_WRITE_TARGET(pipeline_reg);
    SET_WB_WRITE_TARGET(tmp_reg, write_register);
    SET_WB_SIG_WRITE_REG(tmp_reg, signals.reg_write);

    if (signals.reg_write) { // If this had RegWrite set 1, write to register file.
        if (write_register > REGISTER_COUNT && to_store < 0) { // If the register was invalid, quit.
            printf("[ERROR] Invalid register index: %d\n", write_register);
            return -1;
        } else { // If valid, store data.
            ctx->reg_map[write_register] = to_store;
            if (write_register != 0) SET_USED_REG(ctx->used_map, write_register);
            printf("    r[%d] <- 0x%08x\n", write_register, to_store);
        }
    }

    return 0;
}

/**
 * A function that flushes a specific stage.
 * When a stage was flushed, the pipeline registers will be wiped out.
 * @param ctx The current context.
 * @param stage The stage, refer to STAGE_ defines in utils.h
 * @return 0 always.
 */
int new_mips_flush(struct context_t *ctx, uint8_t stage) {
    struct pipeline_register_t *new_pr = &ctx->new_pipeline_reg;
    struct control_signals_t empty = {0};
    switch (stage) {
        case STAGE_IF:
            printf("[Flush] Flushing Stage IF\n");
            SET_IF_ID_INSN(new_pr, 0x00000000);
            SET_IF_ID_PC(new_pr, 0x00000000);
            break;
        case STAGE_ID:
            printf("[Flush] Flushing Stage ID\n");
            SET_ID_EX_INSN(new_pr, 0x00000000);
            SET_ID_EX_CTR_SIG(new_pr, empty);
            SET_ID_EX_PC(new_pr, 0x00000000);
            SET_ID_EX_JUMP_ADDR(new_pr, 0x00000000);
            SET_ID_EX_JUMP_TYPE(new_pr, 0x00000000);
            SET_ID_EX_READ_DATA_1(new_pr, 0x00000000);
            SET_ID_EX_READ_DATA_2(new_pr, 0x00000000);
            SET_ID_EX_RS(new_pr, 0x00000000);
            SET_ID_EX_RT(new_pr, 0x00000000);
            SET_ID_EX_SIGN_EXTEND(new_pr, 0x00000000);
            SET_ID_EX_WRITE_RD(new_pr, 0x00000000);
            SET_ID_EX_WRITE_RT(new_pr, 0x00000000);
            break;
        case STAGE_EX:
            printf("[Flush] Flushing Stage EX\n");
            SET_EX_MEM_CTR_SIG(new_pr, empty);
            SET_EX_MEM_ALU_RESULT(new_pr, 0x00000000);
            SET_EX_MEM_JUMP_ADDR(new_pr, 0x00000000);
            SET_EX_MEM_JUMP_TYPE(new_pr, 0x00000000);
            SET_EX_MEM_PC(new_pr, 0x00000000);
            SET_EX_MEM_PC_ADD_RESULT(new_pr, 0x00000000);
            SET_EX_MEM_READ_DATA_2(new_pr, 0x00000000);
            SET_EX_MEM_WRITE_TARGET(new_pr, 0x00000000);
            SET_EX_MEM_ZERO(new_pr, 0);
            break;
        case STAGE_MEM:
            printf("[Flush] Flushing Stage MEM\n");
            SET_MEM_WB_CTR_SIG(new_pr, empty);
            SET_MEM_WB_ALU_RESULT(new_pr, 0x00000000);
            SET_MEM_WB_PC(new_pr, 0x00000000);
            SET_MEM_WB_READ_DATA(new_pr, 0x00000000);
            SET_MEM_WB_WRITE_TARGET(new_pr, 0x00000000);
            break;
        case STAGE_WB:
            printf("[Flush] Flushing Stage WB\n");
            SET_WB_SIG_WRITE_REG(new_pr, 0);
            SET_WB_WRITE_TARGET(new_pr, 0);
            break;
        default:
            break;
    }

    return 0;
}

/**
 * A function that performs forwarding.
 * This is yet most complicated, spaghetti code.
 * Frankly speaking, the code will not take care of the load-use
 * data hazards. When we use the mips cross compiler, it automatically
 * avoids the load-use data hazard by default.
 * Also, this function enables multiple forwards in a single cycle.
 * For example, if we have lw $v0 0($sp), li $v1 10, bne $v0 $v1 test,
 * both values from li's MEM stage and li's EX stage will be forwarded
 * into the bne instruction.
 * @deprecated
 * @param ctx The current context.
 * @return -1 if failure, 0 if success.
 */
int new_mips_forward(struct context_t *ctx) {
    struct pipeline_register_t *future_pr = &ctx->future_pipeline_reg;
    struct pipeline_register_t *new_pr = &ctx->new_pipeline_reg;

    // Get registers and signals, set forwarding history as none.
    uint8_t ex_mem_reg_write = GET_EX_MEM_CTR_SIG(future_pr).reg_write;
    uint8_t mem_wb_reg_write = GET_MEM_WB_CTR_SIG(future_pr).reg_write;
    uint8_t ex_mem_target_reg = GET_EX_MEM_WRITE_TARGET(future_pr);
    uint8_t mem_wb_target_reg = GET_MEM_WB_WRITE_TARGET(future_pr);
    ctx->before_forward = 0;

    // MEM Hazard, double data hazard, Forward A = 01
    if (mem_wb_reg_write && (mem_wb_target_reg != 0) &&
        !(ex_mem_reg_write && (ex_mem_target_reg != 0) &&
          (ex_mem_target_reg == GET_ID_EX_RS(future_pr))) &&
        (mem_wb_target_reg == GET_ID_EX_RS(future_pr))) {

        uint32_t to_forward = MUX(GET_MEM_WB_READ_DATA(future_pr), GET_MEM_WB_ALU_RESULT(future_pr), GET_MEM_WB_CTR_SIG(future_pr).mem_to_reg);
        SET_ID_EX_READ_DATA_1(new_pr, to_forward);
        printf("[Forwarding] MEM Store -> ReadData 1 (%x)\n", GET_ID_EX_READ_DATA_1(new_pr));
        ctx->before_forward = ctx->before_forward | 0x1;

    }

    // MEM Hazard, double data hazard, Forward B = 01
    if (mem_wb_reg_write && (mem_wb_target_reg != 0) &&
        !(ex_mem_reg_write && (ex_mem_target_reg != 0) &&
          (ex_mem_target_reg == GET_ID_EX_RT(future_pr))) &&
        (mem_wb_target_reg == GET_ID_EX_RT(future_pr))) {

        uint32_t to_forward = MUX(GET_MEM_WB_READ_DATA(future_pr), GET_MEM_WB_ALU_RESULT(future_pr), GET_MEM_WB_CTR_SIG(future_pr).mem_to_reg);
        SET_ID_EX_READ_DATA_2(new_pr, to_forward);
        printf("[Forwarding] MEM Store -> ReadData 2 (%x)\n", GET_ID_EX_READ_DATA_2(new_pr));
        ctx->before_forward = ctx->before_forward | 0x2;
    }

    // EX Hazard, ForwardA = 10
    if (ex_mem_reg_write && (ex_mem_target_reg != 0) && (ex_mem_target_reg == GET_ID_EX_RS(future_pr))) {
        SET_ID_EX_READ_DATA_1(new_pr, GET_EX_MEM_ALU_RESULT(future_pr));
        printf("[Forwarding] ALU Result -> ReadData 1 (%x)\n", GET_ID_EX_READ_DATA_1(new_pr));
        ctx->before_forward = ctx->before_forward | 0x1;
    }

    // EX Hazard, ForwardB = 10
    if (ex_mem_reg_write && (ex_mem_target_reg != 0) && (ex_mem_target_reg == GET_ID_EX_RT(future_pr))) {
        SET_ID_EX_READ_DATA_2(new_pr, GET_EX_MEM_ALU_RESULT(future_pr));
        printf("[Forwarding] ALU Result -> ReadData 2 (%x)\n", GET_ID_EX_READ_DATA_2(new_pr));
        ctx->before_forward = ctx->before_forward | 0x2;
    }

    return 0;
}

/**
 * A function that performs branch prediction.
 * This will generate branch address and set it as the PC for next clock cycle.
 * Actual MIPS implementation for branch prediction will cost one more cycle for
 * calculating the branch address. However, in this program this will be ignored.
 * @param ctx The current context.
 * @param is_bubbled If the ID stage was stalled or not.
 *                   We need to check ID stage was stalled or not.
 *                   If, ID stage was stalled, we need to avoid setting PC + 4.
 *                   This is due the processor having to perform IF again since
 *                   the instruction was not fetched due to stalls.
 * @return -1 if failure, 0 if success.
 */
int new_mips_branch_prediction(struct context_t *ctx, uint8_t is_bubbled) {
    uint8_t res;

    // Generate branch address.
    // This is not correct, since the branch prediction will take one more cycle.
    // However, in this program we will be ignoring this.
    uint32_t instruction = GET_IF_ID_INSN(&ctx->future_pipeline_reg);
    uint32_t tmp_pc = ctx->pc;
    tmp_pc = tmp_pc + (((int32_t)(int16_t)(GET_IMM(instruction))) << 2);

    // Check branch prediction mode and perform accordingly.
    switch (ctx->bp_mode) {
        case BRANCH_MODE_1_BIT: // 1 Bit branch prediction
            res = PREDICT_1_BIT_BRANCH(ctx->branch_history);
            if (res == BRANCH_TAKEN) { // Branch Taken
                printf("[Branch Prediction] Predicting Taken - 1 Bit Predictor (PC: 0x%x)\n", tmp_pc);
                if (!is_bubbled) {
                    ctx->pc = tmp_pc;
                    new_mips_push_last_branch_prediction(ctx, BRANCH_TAKEN);
                }
            } else { // Branch Not taken
                printf("[Branch Prediction] Predicting Not Taken - 1 Bit Predictor (PC: 0x%x)\n", ctx->pc);
                if (!is_bubbled) {
                    new_mips_push_last_branch_prediction(ctx, BRANCH_NOT_TAKEN);
                }
            }
            break;

        case BRANCH_MODE_2_BIT: // 2 Bit branch prediction
            res = PREDICT_2_BIT_BRANCH(ctx->branch_history);
            if (res == BRANCH_TAKEN) { // Branch Taken
                printf("[Branch Prediction] Predicting Taken - 2 Bit Predictor (PC: 0x%x)\n", tmp_pc);
                if (!is_bubbled) {
                    ctx->pc = tmp_pc;
                    new_mips_push_last_branch_prediction(ctx, BRANCH_TAKEN);
                }
            } else { // Branch Not Taken
                printf("[Branch Prediction] Predicting Not Taken - 2 Bit Predictor (PC: 0x%x)\n", ctx->pc);
                if (!is_bubbled) {
                    new_mips_push_last_branch_prediction(ctx, BRANCH_NOT_TAKEN);
                }
            }
            break;

        case BRANCH_MODE_STATIC: // Static branch prediction, will set not taken.
        default:
            if (!is_bubbled) {
                printf("[Branch Prediction] Predicting Not Taken - Static Predictor (PC: 0x%x)\n", tmp_pc);
                new_mips_push_last_branch_prediction(ctx, BRANCH_NOT_TAKEN);
            }
    }

    return 0;
}

/**
 * A function that pushes branch prediction to the stack.
 * In order to validate HIT or MISS of branch prediction,
 * a stack will be used to keep track of the branch
 * prediction results. This will just use a single uint8_t
 * to perform stack instructions. This function will be used
 * when the branch prediction was made.
 *
 * Be aware of the fact that this stack will have size of 8.
 * @param ctx The current context.
 * @param value The value of branch prediction. 1 if taken, 0 if not taken.
 * @return 0 always.
 */
int new_mips_push_last_branch_prediction(struct context_t *ctx, uint8_t value) {
    // A simple stack to push branch predictions.
    // Max capacity is 8 predictions.
    ctx->last_branch_predictions = ctx->last_branch_predictions << 1;
    ctx->last_branch_predictions = ctx->last_branch_predictions | value;
    ctx->last_branch_prediction_count++;

    return 0;
}

/**
 * A function that pops branch prediction from the stack.
 * In order to validate HIT or MISS of branch prediction,
 * a stack will be used to keep track of the branch
 * prediction results. This will just use a single uint8_t
 * to perform stack instructions. This function will be used
 * when validating the branch prediction results.
 *
 * Be aware of the fact that this stack will have size of 8.
 * @param ctx The current context.
 * @return The result of popped branch prediction results.
 */
int new_mips_pop_last_branch_prediction(struct context_t *ctx) {
    // A simple stack to pop branch predictions.
    // Max capacity is 8 predictions.
    int ret = ctx->last_branch_predictions & 0x1;
    ctx->last_branch_predictions = ctx->last_branch_predictions >> 1;
    ctx->last_branch_prediction_count--;

    return ret;
}

/**
 * A function that validates branch prediction.
 * This will be executed in MEM stage and will check if the
 * predicted branch is actually correct or not.
 * Also this function will set up the branch history records
 * based upon the predictions, which will be used later.
 * @param ctx The current context.
 * @param branch_result Actual Hit or Miss.
 * @param real_address The actual address that branch should have taken.
 * @return 0 always.
 */
int new_mips_validate_branch_prediction(struct context_t *ctx, uint8_t branch_result, uint32_t real_address) {
    uint8_t predicted_result = new_mips_pop_last_branch_prediction(ctx);
    int translated_result = branch_result == 0 ? -1 : 1;
    switch (ctx->bp_mode) {
        case BRANCH_MODE_1_BIT:
            if (predicted_result == branch_result) { // Prediction Hit.
#ifdef DEBUG
                printf("[1 Bit Branch Prediction] Hit!\n");
#endif
                UPDATE_1_BIT_BRANCH_HISTORY(ctx->branch_history, translated_result);
                ctx->branch_hit_count++;
            } else { // Branch Miss.
#ifdef DEBUG
                printf("[1 Bit Branch Prediction] Miss!\n");
#endif
                ctx->pc = real_address;
                UPDATE_1_BIT_BRANCH_HISTORY(ctx->branch_history, translated_result);
                new_mips_flush(ctx, STAGE_IF);
                new_mips_flush(ctx, STAGE_ID);
                new_mips_flush(ctx, STAGE_EX);
                ctx->need_branch_prediction = 0;
                ctx->branch_miss_count++;
            }
            break;
        case BRANCH_MODE_2_BIT:
            if (predicted_result == branch_result) { // Prediction Hit.
#ifdef DEBUG
                printf("[2 Bit Branch Prediction] Hit!\n");
#endif
                UPDATE_2_BIT_BRANCH_HISTORY(ctx->branch_history, translated_result);
                ctx->branch_hit_count++;
            } else { // Branch Miss.
#ifdef DEBUG
                printf("[2 Bit Branch Prediction] Miss!\n");
#endif
                ctx->pc = real_address;
                UPDATE_2_BIT_BRANCH_HISTORY(ctx->branch_history, translated_result);
                new_mips_flush(ctx, STAGE_IF);
                new_mips_flush(ctx, STAGE_ID);
                new_mips_flush(ctx, STAGE_EX);
                ctx->need_branch_prediction = 0;
                ctx->branch_miss_count++;
            }
            break;
        case BRANCH_MODE_STATIC:
        default:
            if (predicted_result == branch_result) { // Prediction Hit.
#ifdef DEBUG
                printf("[Static Branch Prediction] Hit!\n");
#endif
                ctx->branch_hit_count++;
            } else { // Branch Miss.
#ifdef DEBUG
                printf("[Static Branch Prediction] Miss!\n");
#endif
                ctx->pc = real_address;
                new_mips_flush(ctx, STAGE_IF);
                new_mips_flush(ctx, STAGE_ID);
                ctx->need_branch_prediction = 0;
                ctx->branch_miss_count++;
            }
            break;
    }

    return 0;
}

/**
 * A function that generates signals from opcode.
 * This is a hand written code, so this might be stupid.
 * This only covers the instructions in green sheet from left column of first page.
 * This excludes floating instructions. Therefore do not try this.
 * @param instruction The instruction to process signals.
 * @return The control signal.
 */
struct control_signals_t generate_control_signal(uint32_t instruction) {
    struct control_signals_t ret = {0};

    uint8_t opcode = GET_OPCODE(instruction);
    uint8_t funct = GET_FUNCT(instruction);

    // Determine RegDst.
    // If instruction type is R then it means the RegDst shall be set as 1.
    ret.reg_dst = IS_R_TYPE(opcode);

    // Determine RegWrite.
    // Except R type, there are some instructions that shall have RegWrite.
    // I am intentionally using switch expression for better readability in the documents.
    // Yes this is stupid, might be missing some instructions.
    // For more information, J type will not be considered as using a RegWrite, PC is not considered a normal register.
    switch (opcode) {
        case 0x00: // If this was R type, then have RegWrite as 1.
        case OPCODE_ADDI: // addi
        case OPCODE_ADDIU: // addiu
        case OPCODE_ANDI: // andi
        case OPCODE_LBU: // lbu
        case OPCODE_LHU: // lhu
        case OPCODE_LL: // ll
        case OPCODE_LUI: // lui
        case OPCODE_LW: // lw
        case OPCODE_ORI: // ori
        case OPCODE_SLTI: // slti
        case OPCODE_SLTIU: // sltiu
            ret.reg_write = GET_FUNCT(instruction) != FUNCT_JR; // Exclude jr, this does not write to register.
            break;
        default:
            ret.reg_write = 0;
    }

    // Determine ALUSrc.
    // I am sure that I type instructions require this as 1, but I am not sure that J type also requires this as 1.
    // Beq and bne is exception, this shall have ALUSrc.
    ret.alu_src = (IS_I_TYPE(opcode) || IS_J_TYPE(opcode)) && ((opcode != OPCODE_BEQ) && (opcode != OPCODE_BNE));

    // Determine MemRead and MemtoReg.
    // Those shall share same signals since they are basically the same operations.
    // I am kind of sure that those lbu, lhu, ll, lui, lw uses this as 1.
    switch (opcode) {
        case OPCODE_LBU:
        case OPCODE_LHU:
        case OPCODE_LL:
        case OPCODE_LUI:
        case OPCODE_LW:
            ret.mem_read = 1;
            ret.mem_to_reg = 1;
            break;
        default:
            ret.mem_read = 0;
            ret.mem_to_reg = 0;
    }

    // Determine MemWrite.
    // Also, kind of sure that sb, sc, sh, sw instructions have this as 1.
    switch (opcode) {
        case OPCODE_SB:
        case OPCODE_SC:
        case OPCODE_SH:
        case OPCODE_SW:
            ret.mem_write = 1;
            break;
        default:
            ret.mem_write = 0;
    }

    // Determine PCSrc (aka branch).
    switch (opcode) {
        case OPCODE_J:
        case OPCODE_JAL:
        case OPCODE_BEQ:
        case OPCODE_BNE:
            ret.branch = 1;
            break;
        default: // Except JR, all shall be set 0.
            ret.branch = funct == FUNCT_JR;
    }

    // Determine ALUop.
    // Reference: https://chayan-memorias.tistory.com/176
    if (opcode == OPCODE_LW || opcode == OPCODE_SW) { // For lw and sw, those two have ALUOp as 00.
        ret.alu_op_0 = 0;
        ret.alu_op_1 = 0;
    } else if (opcode == OPCODE_BEQ || opcode == OPCODE_BNE) { // Shall have ALUop as 01.
        ret.alu_op_0 = 1;
        ret.alu_op_1 = 0;
    } else if (opcode == OPCODE_SLTI || opcode == OPCODE_SLT || opcode == OPCODE_SLTIU) { // slt friends need 10.
        ret.alu_op_0 = 0;
        ret.alu_op_1 = 1;
    } else { // For R types the opcode shall be 10, otherwise, set it 0.
        if (IS_R_TYPE(opcode)) { // R types, set it 10.
            ret.alu_op_0 = 0;
            ret.alu_op_1 = 1;
        } else { // Unknown opcode.
            ret.alu_op_0 = 0;
            ret.alu_op_1 = 0;
        }
    }

    // Determine Jump, J types and include JR.
    ret.jump = IS_J_TYPE(opcode) || (GET_FUNCT(instruction) == FUNCT_JR);

    return ret;
}

/**
 * A function that generates ALU control signal based on the given instruction.
 * This is based on 04-2 Processor's Page 25.
 * @param alu_op_0 The first bit of ALUOp generated from control signal (LSB).
 * @param alu_op_1 The second bit of ALUOp generated from control signal (LSB 1 bit).
 * @param alu_control The pointer to return ALU control signals to.
 * @param instruction The instruction to generate control signal.
 * @return -1 if failure, 0 if success.
 */
int generate_alu_control_signal(uint8_t alu_op_0, uint8_t alu_op_1, uint8_t *alu_control, uint32_t instruction) {
    // Generate 4 bits of ALU control signal.
    // This will start from 0 bit to 4 bit left to right LSB to MSB.
    uint8_t operation;
    uint8_t function_code= GET_FUNCT(instruction);

    // Operation Code 0 is (F0 OR F3) AND ALUOp1.
    operation = ((BIT_0(function_code) | BIT_3(function_code)) & alu_op_1) & 0x1;
    // Operation Code 1 is ~F2 OR ~AluOp1.
    operation = (((~BIT_2(function_code) | ~alu_op_1) & 0x1) << 1) | operation;
    // Operation Code 2 is (F1 AND AluOp1) OR AluOp0.
    operation = ((((BIT_1(function_code) & alu_op_1) | alu_op_0) & 0x1) << 2) | operation;
    // Operation Code 3 is (AluOp0 AND ~AluOp0) shall be 0.
    operation = (((alu_op_0 & ~alu_op_0) & 0x1) << 3) | operation;

    *alu_control = operation;
    return 0;
}

/**
 * A function that performs ALU operation according to the input signals.
 * @param in_1 The first input value.
 * @param in_2 The second input value.
 * @param alu_control The ALU control signal.
 * @param zero The pointer to store zero value.
 * @param alu_result The pointer to store ALU result.
 * @param instruction The instruction to execute in ALU.
 * @return -1 if failure, 0 if success.
 */
int alu(uint32_t in_1, uint32_t in_2, uint8_t alu_control, uint8_t *zero, uint32_t *alu_result, uint32_t instruction) {
    // Perform ALU operation based upon the ALU control signal.
    switch (alu_control) {
        case 0b0011: // Addu. @todo needs verification, since we do not care about traps, we will treat it the same.
        case 0b0010: // Add.
#ifdef DEBUG
            printf("    [DEBUG] ALU add, 0x%08x + 0x%08x = 0x%08x(%d)\n", in_1, in_2, in_1 + in_2, in_1 + in_2);
#endif
            *zero = 0;
            *alu_result = in_1 + in_2;

            // This is not 100% correct in terms of architecture, but
            // in order to support mult this shall be done.
            // This is inevitable :)
            if (GET_FUNCT(instruction) == FUNCT_MULT && IS_R_TYPE(GET_OPCODE(instruction))) {
#ifdef DEBUG
                printf("    [DEBUG] Performing mult, 0x%08x * 0x%08x = 0x%08x(%d)\n", in_1, in_2, in_1 * in_2, in_1 * in_2);
#endif
                *zero = 0;
                *alu_result = 0;
                uint64_t tmp;
                tmp = in_1 * in_2;
                mul_lo = tmp & 0xFFFFFFFF; // Store lower 32 bits.
                mul_hi = (tmp & 0xFFFFFFFF00000000) >> 32; // Store upper 32 bits.
            }

            break;
        case 0b0110: // Subtract.
#ifdef DEBUG
            printf("    [DEBUG] ALU sub, 0x%08x - 0x%08x = 0x%08x(%d)\n", in_1, in_2, in_1 - in_2, in_1 - in_2);
#endif
            *alu_result = in_1 - in_2;
            *zero = 0;

            // For beq check results.
            if (GET_OPCODE(instruction) == OPCODE_BEQ) {
                *zero = *alu_result == 0;
            }

            // For bne, check results.
            if (GET_OPCODE(instruction) == OPCODE_BNE) {
                *zero = *alu_result != 0;
            }

            // Slti comes here since the alu instruction is sub.
            if (GET_OPCODE(instruction) == OPCODE_SLTI || GET_OPCODE(instruction) == OPCODE_SLT) {
#ifdef DEBUG
                printf("    [DEBUG] slt sub, 0x%08x - 0x%08x = 0x%08x(%d) < 0 ? = %d\n", in_1, in_2, in_1 - in_2, in_1 - in_2, in_1 - in_2 < 0);
#endif
                *zero = 0;
                *alu_result = (int)*alu_result < 0;
            }

            // Implement mfhi.
            if (GET_FUNCT(instruction) == FUNCT_MFHI && IS_R_TYPE(GET_OPCODE(instruction))) {
#ifdef DEBUG
                printf("    [DEBUG] Performing mfhi, value: 0x%08x\n", mul_hi);
#endif
                *zero = 0;
                *alu_result = mul_hi;
            }

            // Implement mflo.
            if (GET_FUNCT(instruction) == FUNCT_MFLO && IS_R_TYPE(GET_OPCODE(instruction))) {
#ifdef DEBUG
                printf("    [DEBUG] Performing mflo, value: 0x%08x\n", mul_lo);
#endif
                *zero = 0;
                *alu_result = mul_lo;
            }

            break;
        case 0b0000: // AND.
#ifdef DEBUG
            printf("    [DEBUG] ALU AND, 0x%08x & 0x%08x = 0x%08x(%d)\n", in_1, in_2, in_1 & in_2, in_1 & in_2);
#endif
            *zero = 0;
            *alu_result = in_1 & in_2;
            break;
        case 0b0001: // OR.
#ifdef DEBUG
            printf("    [DEBUG] ALU OR, 0x%08x | 0x%08x = %0x08x(%d)\n", in_1, in_2, in_1 | in_2, in_1 | in_2);
#endif
            *zero = 0;
            *alu_result =  in_1 | in_2;
            break;
        case  0b0111: // Set on less than. @todo needs verification.
#ifdef DEBUG
            printf("    [DEBUG] SLT, 0x%08x < 0x%08x = %d\n", in_1, in_2, in_1 < in_2);
#endif

            *zero = 0;
            *alu_result = in_1 < in_2;
            break;
        default: // Something got messed up.
            return -1;
    }
    return 0;
}

/**
 * A function that prints out the used registers of current execution.
 * @param ctx The current context.
 */
void print_used_registers(struct context_t *ctx) {
    char reg_name[8] = {0};
    uint8_t used_count = 0;
    for (uint8_t i = 0; i < 32 ; i++) {
        if (IS_USED_REG(ctx->used_map, i)) {
            get_register_name(i, reg_name);
            printf("%s(%d): 0x%08x(%d), ", reg_name, i, ctx->reg_map[i], ctx->reg_map[i]);
            used_count++;
        }
    }
    if (used_count != 0) {
        printf("\n");
    } else {
        printf("None\n");
    }
}

/**
 * A function that retrieves the register's name by register index.
 * If the register was not found, this will return "N/A".
 * @param reg_index The register index to translate into register name.
 * @param ret The pointer to store register name into.
 * @return -1 if failure, 0 if success.
 */
int get_register_name(uint8_t reg_index, char *ret) {
    char buf[8] = {0};

    if (reg_index == 0) { // $zero
        memcpy(ret, "$zero", 6);
    } else if (reg_index == 1) { // $at
        memcpy(ret, "$at", 4);
    } else if (reg_index >= 2 && reg_index <= 3) { // $v0 ~ $v1
        sprintf(buf, "$v%d", reg_index - 2);
        memcpy(ret, buf, sizeof(buf));
    } else if (reg_index >= 4 && reg_index <= 7) { // $a0 ~ a3
        sprintf(buf, "$a%d", reg_index - 4);
        memcpy(ret, buf, sizeof(buf));
    } else if (reg_index >= 8 && reg_index <= 15) { // $t0 ~ $t7
        sprintf(buf, "$t%d", reg_index - 8);
        memcpy(ret, buf, sizeof(buf));
    } else if (reg_index >= 16 && reg_index <= 23) { // $s0 ~ $s7
        sprintf(buf, "$s%d", reg_index - 16);
        memcpy(ret, buf, sizeof(buf));
    } else if (reg_index >= 24 && reg_index <=  25) { // $t8, $t9
        sprintf(buf, "$t%d", reg_index - 16);
        memcpy(ret, buf, sizeof(buf));
    } else if (reg_index >= 26 && reg_index <= 27) { // $k0, $k1
        sprintf(buf, "$k%d", reg_index - 26);
        memcpy(ret, buf, sizeof(buf));
    } else if (reg_index == 28) {
        memcpy(ret, "$gp", 4);
    } else if (reg_index == 29) {
        memcpy(ret, "$sp", 4);
    } else if (reg_index == 30) {
        memcpy(ret, "$fp", 4);
    } else if (reg_index == 31) {
        memcpy(ret, "$ra", 4);
    } else {
        memcpy(ret, "N/A", 4);
        return -1;
    }
    return 0;
}
