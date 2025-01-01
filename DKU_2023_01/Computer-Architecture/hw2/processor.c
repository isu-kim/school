//
// @file : processor.c
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that implements processor related functions.
//

#include "processor.h"

uint32_t cpu_instruction = 0x00000000; // The current loaded instruction in CPU.
uint32_t mul_hi = 0x00000000;
uint32_t mul_lo = 0x00000000;

/**
 * A function that executes the stored process.
 * @param ctx The current context.
 * @return 0 if success, -1 if failure.
 */
int run(struct context_t *ctx) {
    // Initialize clock cycle count.
    ctx->clock_count = 1;

    // While the instructions are not finished, do cycles.
    while (!is_finished(ctx)) {
        if (one_cycle(ctx) == -1) {
            printf("[ERROR] Could not execute cycle: %d\n", ctx->clock_count);
            return -1;
        }
    }

    // The execution terminated successfully.
    printf("[INFO] The execution terminated successfully\n");
    return 0;
}

/**
 * A function that executes code for a cycle.
 * @param ctx The current context.
 * @return 0 if this clock cycle was performed successfully, -1 if failure.
 */
int one_cycle(struct context_t *ctx) {
    int ret;
    ctx->reg_map[ZERO] = 0x00000000; // $zero is hard wired to 0, so need to fix value.

    // Show clock and PC information first.
    printf("-------==[ Clock %d ]==-------\n [ PC: 0x%08x / SP: 0x%08x / RA: 0x%08x ]\n", ctx->clock_count, ctx->pc, ctx->reg_map[SP], ctx->reg_map[RA]);
    printf("Used Registers:\n   - ");
    print_used_registers(ctx);
    struct control_signals_t signals = {0}; // For control signals from decode stage.
    
    // Instruction Fetch.
    ret = mips_fetch(ctx);
    if (ret != 0) {
        return ret;
    }

    // Instruction Decode.
    ret = mips_decode(ctx, &signals);
    if (ret != 0) {
        return ret;
    }

    // Instruction Execute.
    ret = mips_execute(ctx, signals);
    if (ret != 0) {
        return ret;
    }

    // Read and Write data from Memory.
    ret = mips_load_store(ctx, signals);
    if (ret != 0) {
        return ret;
    }

    // Write data to Register File.
    ret = mips_write_back(ctx, signals);
    if (ret != 0) {
        return ret;
    }

    ctx->clock_count++;
    return 0;
}

/**
 * A function that mimics "Instruction Fetch" step in mips processor.
 * Fetch will move instruction from memory to CPU for future procedure.
 * @param ctx The current context.
 * @return 0 if success, -1 if failure.
 */
int mips_fetch(struct context_t *ctx) {
    // Load instruction in memory to CPU.
    // We need to convert big endian to little endian since the file was written in big.
    uint32_t tmp_instruction = BIG_TO_LITTLE_32(ctx->mem[CONVERT_PC(ctx->pc)]);
    cpu_instruction = tmp_instruction;
    // Increment PC, yes this is not always the case but, in this code we will do this.
    ctx->pc = ctx->pc + 4;
    printf("1) Instruction Fetch - Instruction: 0x%08x\n", tmp_instruction);
    return 0;
}

/**
 * A function that mimics "Instruction Decode" step in mips processor.
 * Instruction decoding performs following steps:
 * - Generate control signal based upon the instruction.
 * - Read register map and output Read data 1 and Read data 2.
 * - Extend immediate value to 32 bits.
 * @todo Verify this actually works.
 * @param ctx The current context.
 * @param signals The pointer to store processed signals.
 * @return 0 if success, -1 if failure.
 */
int mips_decode(struct context_t *ctx, struct control_signals_t *signals) {
    // Generate control signal, then copy it to return signal.
    struct control_signals_t sig = {0};
    sig = generate_control_signal(cpu_instruction);
    memcpy(signals, &sig, sizeof(struct control_signals_t));

    // Load registers.
    ctx->reg_read_register_1 = GET_RS(cpu_instruction);
    ctx->reg_read_register_2 = GET_RT(cpu_instruction);
    ctx->read_data_1 = ctx->reg_map[ctx->reg_read_register_1]; // Retrieve ReadData1.
    ctx->read_data_2 = ctx->reg_map[ctx->reg_read_register_2]; // Retrieve ReadData2.
    ctx->reg_extended_immediate = (int32_t)(int16_t)GET_IMM(cpu_instruction); // Extend 16bit to 32bit but keep sign.

#ifdef DEBUG
    printf("[DEBUG] OPCODE(%x): \n"
           "   - Signals: RegDst=%0x,AluSrc=%0x,MemToReg=%0x,RegWrite=%0x,MemRead=%0x,MemWrite=%0x,Branch=%0x,AluOp0=%0x,AluOp1=%0x,Jump=%0x\n",
           GET_OPCODE(cpu_instruction), signals->reg_dst, signals->alu_src, signals->mem_to_reg, signals->reg_write,
           signals->mem_read, signals->mem_write, signals->branch, signals->alu_op_0, signals->alu_op_1, signals->jump);
#endif

    // Set information accordingly.
    char buff[40] = {0};
    uint8_t opcode = GET_OPCODE(cpu_instruction);
    if (IS_R_TYPE(opcode)) {
        sprintf(buff, "Type: R / OPCODE: 0x%x / FUNCT: 0x%x", GET_OPCODE(cpu_instruction), GET_FUNCT(cpu_instruction));
        ctx->instruction_type_counts[INSTRUCTION_TYPE_R]++;
    } else if (IS_J_TYPE(opcode)) {
        sprintf(buff, "Type: J / OPCODE: 0x%x", GET_OPCODE(cpu_instruction));
        ctx->instruction_type_counts[INSTRUCTION_TYPE_J]++;
    } else if (IS_I_TYPE(opcode)) {
        sprintf(buff, "Type: I / OPCODE: 0x%x", GET_OPCODE(cpu_instruction));
        ctx->instruction_type_counts[INSTRUCTION_TYPE_I]++;
    }
    
    printf("2) Instruction Decoding - %s \n"
           "   - ReadData1: 0x%08x(%d) / ReadData2: 0x%08x(%d) / Extended Imm: 0x%08x(%d)\n",
           buff, ctx->read_data_1, ctx->read_data_1, ctx->read_data_2, ctx->read_data_2, ctx->reg_extended_immediate, ctx->reg_extended_immediate);
    return 0;
}

/**
 * A function that mimics "Execute" step in mips processor.
 * This function will perform ALU operation using ALU control.
 * Also this will calculate jump offsets with immediate value and store new jump address, then on conditions, this will
 * update the current PC whether to jump or not.
 * @param ctx The current context.
 * @param signals The current control signals.
 * @return -1 if failure, 0 if success.
 */
int mips_execute(struct context_t *ctx, struct control_signals_t signals) {
    // Check ALUSrc for determining whether this shall use RT or immediate.
    uint32_t mux_out = MUX(ctx->reg_extended_immediate, ctx->read_data_2, signals.alu_src);
    // Generate ALU control signal from ALU control based upon the diagram.
    uint8_t alu_control;
    int ret = generate_alu_control_signal(signals.alu_op_0, signals.alu_op_1, &alu_control);
    if (ret != 0) { // Something went terribly wrong.
        printf("[ERROR] Could not generate ALU control signal: 0x%08x\n", cpu_instruction);
        return -1;
    }

    // Get return values from the ALU.
    uint8_t zero = 0;
    ret = alu(ctx->read_data_1, mux_out, alu_control, &zero, &ctx->alu_result);
    if (ret != 0) {
        printf("[ERROR] Could not execute instruction in ALU: 0x%08x\n", cpu_instruction);
        return -1;
    }

    // Determine whether or to use PC + 4 or PC = address using MUX and control signals.
    // Print out the results.
    printf("3) Execute - ALU Control (0x%x)\n", alu_control);
    printf("   - ALU Result: 0x%08x / Zero %d\n", ctx->alu_result, zero);
    uint32_t tmp_pc;

    // Check branch and target address. If this PC + 4 or if this is branch.
    if (((signals.branch & zero) & 0x1) == 1) {
        uint32_t jump_offset = ctx->reg_extended_immediate << 2;
        printf("   - Branch Taken, Offset PC = 0x%08x + 0x%08x\n", ctx->pc, jump_offset);
        tmp_pc = ctx->pc + jump_offset;
        ctx->branch_taken_count++;
    } else {
        tmp_pc = ctx->pc;
    }

    // For Jump instruction, generate address using address | PC.
    uint32_t jump_addr = GET_ADDR(cpu_instruction);
    jump_addr = jump_addr << 2; // Shift left two times for jump address.
    jump_addr = jump_addr | GET_PC_UPPER(tmp_pc); // Get upper bits from PC and make full jump address.
    if (signals.jump) {
	if (GET_FUNCT(cpu_instruction) == FUNCT_JR) {
	    ctx->pc = ctx->alu_result;
	}
        if (GET_OPCODE(cpu_instruction) == OPCODE_JAL) { // jal shall store current PC.
            ctx->reg_map[RA] = ctx->pc;
            ctx->pc = jump_addr;
	}
        printf("   - Jump taken, PC = 0x%08x\n", ctx->pc);
    } else {
        printf("   - Normal Case, PC = PC + 4\n");
        ctx->pc = tmp_pc;
    }

    printf("   - Update new PC: 0x%08x\n", ctx->pc);

    return 0;
}

/**
 * A function that mimics "Load and Store to memory" step in mips processor.
 * MemRead and MemWrite cannot be taken place at the same place but this function will NOT take care of the exception.
 * We need to be extra cautious about the segfault since this is the point where this program might get messed up.
 * @param ctx The current context.
 * @param signals The current control signals.
 * @return -1 if failure, 0 if success.
 */
int mips_load_store(struct context_t *ctx, struct control_signals_t signals) {
    // Check memory boundary if this was valid memory address.
    // This is the last method of protecting this program.
    // Below here is just memcpy without any checks.
    if ((signals.mem_read || signals.mem_write) && ctx->alu_result > MAX_BIN_SIZE) {
        printf("[ERROR] Given address is invalid: 0x%08x\n", ctx->alu_result);
        return -1;
    }

    char* offset = (char *)ctx->mem; // Need to typecast since we are going to use per bytes.
    void* ret = NULL;
    offset = offset + ctx->alu_result; // Calculate offset in memory.

    if (signals.mem_write) { // If this had MemWrite signal turned on.
        printf("4) Memory Write - Address(0x%08x)\n", ctx->alu_result);
        ret = memcpy(offset, &ctx->read_data_2, 4); // Store data to memory.
        if (!ret) { // memcpy failed.
            return -1;
        }
        printf("   - Successfully written 0x%08x to Address 0x%08x\n", ctx->read_data_2, ctx->alu_result);
        ctx->mem_access_count++;
    }

    if (signals.mem_read) { // If this had MemRead signal turned on.
        printf("4) Memory Read - Address(0x%08x)\n", ctx->alu_result);
        ret = memcpy(&ctx->mem_read_data, offset, 4); // Read data from memory.
        if (!ret) { // memcpy failed.
            return -1;
        }
        printf("   - Successfully Read 0x%08x from Address 0x%08x\n", ctx->mem_read_data, ctx->alu_result);
        ctx->mem_access_count++;
    }

    return 0;
}

/**
 * A function that mimics "Register Write Back" step in mips processor.
 * This will write data into the register file.
 * @param ctx The current context.
 * @param signals The current control signals.
 * @return -1 if failure, 0 if success.
 */
int mips_write_back(struct context_t *ctx, struct control_signals_t signals) {
    // Determine the data to store, whether this is a data from memory or this is a data from ALU result.
    uint32_t to_store = MUX(ctx->mem_read_data, ctx->alu_result, signals.mem_to_reg);
    // Determine where to write register, whether this is a register RD or RT register.
    uint8_t write_register = MUX(GET_RD(cpu_instruction), GET_RT(cpu_instruction), signals.reg_dst);

    if (signals.reg_write) { // If this had RegWrite set 1, write to register file.
        printf("5) Register Write\n   - Target Register: %d / Value: 0x%08x\n", write_register, to_store);
        if (write_register > REGISTER_COUNT || to_store < 0) { // If the register was invalid, quit.
            printf("[ERROR] Invalid register index: %d\n", write_register);
            return -1;
        } else { // If valid, store data.
            ctx->reg_map[write_register] = to_store;
            if (write_register != 0) SET_USED_REG(ctx->used_map, write_register);
        }
    }

    return 0;
}

/**
 * A function that checks if the execution was finished or not.
 * @param ctx The current context.
 * @return 1 if this program shall be terminated, 0 if not.
 */
int is_finished(struct context_t *ctx) {
    return ctx->pc == 0xFFFFFFFF; // According to requirements, this is the termination.
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
            ret.reg_write = GET_FUNCT(cpu_instruction) != FUNCT_JR; // Exclude jr, this does not write to register.
            break;
        default:
            ret.reg_write = 0;
    }

    // Determine ALUSrc.
    // I am sure that I type instructions require this as 1, but I am not sure that J type also requires this as 1.
    // Beq and bne is exception, this shall have ALUSrc.
    // @todo Needs verification for J type.
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
    ret.jump = IS_J_TYPE(opcode) || (GET_FUNCT(cpu_instruction) == FUNCT_JR);

    return ret;
}

/**
 * A function that generates ALU control signal based on the given instruction.
 * This is based on 04-2 Processor's Page 25.
 * @param alu_op_0 The first bit of ALUOp generated from control signal (LSB).
 * @param alu_op_1 The second bit of ALUOp generated from control signal (LSB 1 bit).
 * @param alu_control The pointer to return ALU control signals to.
 * @return -1 if failure, 0 if success.
 */
int generate_alu_control_signal(uint8_t alu_op_0, uint8_t alu_op_1, uint8_t *alu_control) {
    // Generate 4 bits of ALU control signal.
    // This will start from 0 bit to 4 bit left to right LSB to MSB.
    uint8_t operation;
    uint8_t function_code= GET_FUNCT(cpu_instruction);

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
 * @return -1 if failure, 0 if success.
 */
int alu(uint32_t in_1, uint32_t in_2, uint8_t alu_control, uint8_t *zero, uint32_t *alu_result) {
    // Perform ALU operation based upon the ALU control signal.
    switch (alu_control) {
        case 0b0011: // Addu. @todo needs verification, since we do not care about traps, we will treat it the same.
        case 0b0010: // Add.
#ifdef DEBUG
            printf("[DEBUG] ALU add, 0x%08x + 0x%08x = 0x%08x(%d)\n", in_1, in_2, in_1 + in_2, in_1 + in_2);
#endif
            *zero = 0;
            *alu_result = in_1 + in_2;

	    // This is not 100% correct in terms of architecture, but
	    // in order to support mult this shall be done.
	    // This is inevitable :)
	    if (GET_FUNCT(cpu_instruction) == FUNCT_MULT && IS_R_TYPE(GET_OPCODE(cpu_instruction))) {
#ifdef DEBUG
	    printf("[DEBUG] Performing mult, 0x%08x * 0x%08x = 0x%08x(%d)\n", in_1, in_2, in_1 * in_2, in_1 * in_2);
#endif
		*zero = 0;
		*alu_result = 0;
		uint64_t tmp;
		tmp = in_1 * in_2;
		mul_lo = tmp & 0xFFFFFFFF; // Store lower 32 bits.
		mul_hi = (tmp & 0xFFFFFFFF00000000) >> 32; // Store hupper 32 bits.
	    }

            break;
        case 0b0110: // Subtract.
#ifdef DEBUG
            printf("[DEBUG] ALU sub, 0x%08x - 0x%08x = 0x%08x(%d)\n", in_1, in_2, in_1 - in_2, in_1 - in_2);
#endif
            *alu_result = in_1 - in_2;
            *zero = 0;

            // For beq check results.
            if (GET_OPCODE(cpu_instruction) == OPCODE_BEQ) {
                *zero = *alu_result == 0;
            }

            // For bne, check results.
            if (GET_OPCODE(cpu_instruction) == OPCODE_BNE) {
                *zero = *alu_result != 0;
            }

	    // Slti comes here since the alu instruction is sub.
	    if (GET_OPCODE(cpu_instruction) == OPCODE_SLTI || GET_OPCODE(cpu_instruction) == OPCODE_SLT) {
#ifdef DEBUG
            printf("[DEBUG] slt sub, 0x%08x - 0x%08x = 0x%08x(%d) < 0 ? = %d\n", in_1, in_2, in_1 - in_2, in_1 - in_2, in_1 - in_2 < 0);
#endif
		*zero = 0;
		*alu_result = (int)*alu_result < 0;
	    }

	    // Implement mfhi.
	    if (GET_FUNCT(cpu_instruction) == FUNCT_MFHI && IS_R_TYPE(GET_OPCODE(cpu_instruction))) {
#ifdef DEBUG
	    printf("[DEBUG] Performing mfhi, value: 0x%08x\n", mul_hi);
#endif
		*zero = 0;
		*alu_result = mul_hi;
	    }

	    // Implement mflo.
	    if (GET_FUNCT(cpu_instruction) == FUNCT_MFLO && IS_R_TYPE(GET_OPCODE(cpu_instruction))) {
#ifdef DEBUG
	    printf("[DEBUG] Performing mflo, value: 0x%08x\n", mul_lo);
#endif
		*zero = 0;
		*alu_result = mul_lo;
	    }

            break;
        case 0b0000: // AND.
#ifdef DEBUG
            printf("[DEBUG] ALU AND, 0x%08x & 0x%08x = 0x%08x(%d)\n", in_1, in_2, in_1 & in_2, in_1 & in_2);
#endif
            *zero = 0;
            *alu_result = in_1 & in_2;
            break;
        case 0b0001: // OR.
#ifdef DEBUG
            printf("[DEBUG] ALU OR, 0x%08x | 0x%08x = %0x08x(%d)\n", in_1, in_2, in_1 | in_2, in_1 | in_2);
#endif
            *zero = 0;
            *alu_result =  in_1 | in_2;
            break;
        case  0b0111: // Set on less than. @todo needs verification.
#ifdef DEBUG
            printf("[DEBUG] SLT, 0x%08x < 0x%08x = %d\n", in_1, in_2, in_1 < in_2);
#endif
            *zero = in_1 < in_2;
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
