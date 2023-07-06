//
// @file : utils.h
// @author : Isu Kim @ isu@isu.kim
// @brief : A file that defines macros for utility features.
//

#ifndef CA_HW2_UTILS_H
#define CA_HW2_UTILS_H

// Define opcodes.
#define OPCODE_ADD 0x0
#define OPCODE_ADDI 0x8
#define OPCODE_ADDIU 0x9
#define OPCODE_ADDU 0x0
#define OPCODE_AND 0x0
#define OPCODE_ANDI 0xc
#define OPCODE_BEQ 0x4
#define OPCODE_BNE 0x5
#define OPCODE_J 0x2
#define OPCODE_JAL 0x3
#define OPCODE_JR 0x0
#define OPCODE_LBU 0x24
#define OPCODE_LHU 0x25
#define OPCODE_LL 0x30
#define OPCODE_LUI 0xF
#define OPCODE_LW 0x23
#define OPCODE_NOR 0x0
#define OPCODE_OR 0x0
#define OPCODE_ORI 0xd
#define OPCODE_SLT 0x0
#define OPCODE_SLTI 0xa
#define OPCODE_SLTIU 0xb
#define OPCODE_SLL 0x0
#define OPCODE_SRL 0x0
#define OPCODE_SB 0x28
#define OPCODE_SC 0x38
#define OPCODE_SH 0x29
#define OPCODE_SW 0x2b
#define OPCODE_SUB 0x0
#define OPCODE_SUBU 0x0

// Define function codes.
#define FUNCT_ADD 0x20
#define FUNCT_ADDU 0x21
#define FUNCT_AND 0x24
#define FUNCT_JR 0x08
#define FUNCT_NOR 0x27
#define FUNCT_OR 0x25
#define FUNCT_SLT 0x2a
#define FUNCT_SLTU 0x2b
#define FUNCT_SLL 0x00
#define FUNCT_SRL 0x02
#define FUNCT_SUB 0x22
#define FUNCT_SUBU 0x23

// Define multiplication, this is not 100% correct but to add the instruction.
#define FUNCT_MULT 0x18
#define FUNCT_MFHI 0x10
#define FUNCT_MFLO 0x12

// Define pipeline register's indexes.
#define IF_ID_PC 0
#define IF_ID_INSN 1
#define ID_EX_INSN 2
#define ID_EX_PC 3
#define ID_EX_READ_DATA_1 4
#define ID_EX_READ_DATA_2 5
#define ID_EX_SIGN_EXTEND 6
#define ID_EX_WRITE_RT 7
#define ID_EX_WRITE_RD 8
#define ID_EX_JUMP_ADDR 9
#define ID_EX_JUMP_TYPE 10
#define ID_EX_RS 11
#define ID_EX_RT 12
#define EX_MEM_PC_ADD_RESULT 13
#define EX_MEM_ALU_RESULT 14
#define EX_MEM_READ_DATA_2 15
#define EX_MEM_WRITE_TARGET 16
#define EX_MEM_JUMP_ADDR 17
#define EX_MEM_JUMP_TYPE 18
#define EX_MEM_PC 19
#define MEM_WB_READ_DATA 20
#define MEM_WB_ALU_RESULT 21
#define MEM_WB_WRITE_TARGET 22
#define MEM_WB_PC 23
#define WB_WRITE_TARGET 24
#define WB_SIG_WRITE_REG 25

// Define jump type.
#define JUMP_JAL 1
#define JUMP_JR 2

// Define elements for branch prediction.
#define BRANCH_HIT 1
#define BRANCH_MISS -1
#define BRANCH_TAKEN 1
#define BRANCH_NOT_TAKEN 0

// Define stage types.
#define STAGE_IF 0
#define STAGE_ID 1
#define STAGE_EX 2
#define STAGE_MEM 3
#define STAGE_WB 4

// Macros for future use :)
// For converting big endian to little endian.
#define BIG_TO_LITTLE_32(x) (((x) >> 24) | (((x) >> 8) & 0xff00) | (((x) << 8) & 0xff0000) | ((x) << 24))
// For converting pc address to the memory's index.
#define CONVERT_PC(x) ((x) / 4) // Kind of not sure whether to deduct 0x10000000.
// For getting opcode from the instruction.
#define GET_OPCODE(x) (((x) & 0xFC000000) >> 26)
// For getting funct code from the instruction.
#define GET_FUNCT(x) (((x) & 0x3F))
// For getting shift amount from the instruction.
#define GET_SHAMT(x) (((x) & 0x7C0) >> 6)
// For checking instruction R type.
#define IS_R_TYPE(opcode) (((opcode) == 0x00000))
// For checking instruction J type.
#define IS_J_TYPE(opcode) (((opcode) == 0x02) || ((opcode) == 0x03))
// For checking instruction I type.
#define IS_I_TYPE(opcode) (!((IS_R_TYPE(opcode)) || (IS_J_TYPE(opcode))))
// For getting RS register value from instruction (25 ~ 21 bit).
#define GET_RS(instruction) (((instruction) & 0x03E00000) >> 21)
// For getting RT register value from instruction (20 ~ 16 bit).
#define GET_RT(instruction) (((instruction) & 0x001F0000) >> 16)
// For getting RD register value from instruction (15 ~ 11 bit).
#define GET_RD(instruction) (((instruction) & 0x0000F800) >> 11)
// For getting Immediate value from instruction (15 ~ 0 bit).
#define GET_IMM(instruction) (((instruction) & 0x0000FFFF))
// For getting the jump address from instruction (25 ~ 0 bit).
#define GET_ADDR(instruction) (((instruction) & 0x03FFFFFF))
// For getting the PC's upper 31~28 bit.
#define GET_PC_UPPER(pc) ((pc) & 0xF0000000)
// For mimicking MUX operation.
#define MUX(a, b, in) (((in) ? (a) : (b)))

// For retrieving the 0th bit from an input.
#define BIT_0(input) (((input) & 0x00000001) >> 0)
// For retrieving the 1st bit from an input.
#define BIT_1(input) (((input) & 0x00000002) >> 1)
// For retrieving the 2nd bit from an input.
#define BIT_2(input) (((input) & 0x00000004) >> 2)
// For retrieving the 3rd bit from an input.
#define BIT_3(input) (((input) & 0x00000008) >> 3)
// For retrieving the 4th bit from an input.
#define BIT_4(input) (((input) & 0x00000010) >> 4)
// For retrieving the 5th bit from an input.
#define BIT_5(input) (((input) & 0x00000020) >> 5)
// For setting a register as used.
#define SET_USED_REG(bitmap, n) ((bitmap) = ((0x1 << (n)) | (bitmap)))
// For getting if a register is being used or not.
#define IS_USED_REG(bitmap, n) ((((bitmap) >> (n)) & 0x1) == 0x1)

// Define a bunch of macros for retrieving each pipeline register's values.
#define GET_IF_ID_PC(p_reg) (((p_reg)->bit_32_regs[IF_ID_PC]))
#define GET_IF_ID_INSN(p_reg) (((p_reg)->bit_32_regs[IF_ID_INSN]))

#define GET_ID_EX_PC(p_reg) (((p_reg)->bit_32_regs[ID_EX_PC]))
#define GET_ID_EX_INSN(p_reg) (((p_reg)->bit_32_regs[ID_EX_INSN]))
#define GET_ID_EX_READ_DATA_1(p_reg) (((p_reg)->bit_32_regs[ID_EX_READ_DATA_1]))
#define GET_ID_EX_READ_DATA_2(p_reg) (((p_reg)->bit_32_regs[ID_EX_READ_DATA_2]))
#define GET_ID_EX_SIGN_EXTEND(p_reg) (((p_reg)->bit_32_regs[ID_EX_SIGN_EXTEND]))
#define GET_ID_EX_JUMP_ADDR(p_reg) (((p_reg)->bit_32_regs[ID_EX_JUMP_ADDR]))
#define GET_ID_EX_WRITE_RT(p_reg) (((p_reg)->bit_32_regs[ID_EX_WRITE_RT]))
#define GET_ID_EX_WRITE_RD(p_reg) (((p_reg)->bit_32_regs[ID_EX_WRITE_RD]))
#define GET_ID_EX_JUMP_TYPE(p_reg) (((p_reg)->bit_32_regs[ID_EX_JUMP_TYPE]))
#define GET_ID_EX_RS(p_reg) (((p_reg)->bit_32_regs[ID_EX_RS]))
#define GET_ID_EX_RT(p_reg) (((p_reg)->bit_32_regs[ID_EX_RT]))

#define GET_EX_MEM_PC_ADD_RESULT(p_reg) (((p_reg)->bit_32_regs[EX_MEM_PC_ADD_RESULT]))
#define GET_EX_MEM_ALU_RESULT(p_reg) (((p_reg)->bit_32_regs[EX_MEM_ALU_RESULT]))
#define GET_EX_MEM_READ_DATA_2(p_reg) (((p_reg)->bit_32_regs[EX_MEM_READ_DATA_2]))
#define GET_EX_MEM_JUMP_ADDR(p_reg) (((p_reg)->bit_32_regs[EX_MEM_JUMP_ADDR]))
#define GET_EX_MEM_JUMP_TYPE(p_reg) (((p_reg)->bit_32_regs[EX_MEM_JUMP_TYPE]))
#define GET_EX_MEM_WRITE_TARGET(p_reg) (((p_reg)->bit_32_regs[EX_MEM_WRITE_TARGET]))
#define GET_EX_MEM_PC(p_reg) (((p_reg)->bit_32_regs[EX_MEM_PC]))
#define GET_EX_MEM_ZERO(p_reg) (((p_reg)->bit_8_regs[0]))

#define GET_MEM_WB_READ_DATA(p_reg) (((p_reg)->bit_32_regs[MEM_WB_READ_DATA]))
#define GET_MEM_WB_ALU_RESULT(p_reg) (((p_reg)->bit_32_regs[MEM_WB_ALU_RESULT]))
#define GET_MEM_WB_WRITE_TARGET(p_reg) (((p_reg)->bit_32_regs[MEM_WB_WRITE_TARGET]))
#define GET_MEM_WB_PC(p_reg) (((p_reg)->bit_32_regs[MEM_WB_PC]))

#define GET_WB_WRITE_TARGET(p_reg) (((p_reg)->bit_32_regs[WB_WRITE_TARGET]))
#define GET_WB_SIG_WRITE_REG(p_reg) (((p_reg)->bit_32_regs[WB_SIG_WRITE_REG]))

#define GET_ID_EX_CTR_SIG(p_reg) (((p_reg)->sigs[1]))
#define GET_EX_MEM_CTR_SIG(p_reg) (((p_reg)->sigs[2]))
#define GET_MEM_WB_CTR_SIG(p_reg) (((p_reg)->sigs[3]))

// Define a bunch of macros for storing each pipeline register values.
#define SET_IF_ID_PC(p_reg, val) (((p_reg)->bit_32_regs[IF_ID_PC]) = (val))
#define SET_IF_ID_INSN(p_reg, val) (((p_reg)->bit_32_regs[IF_ID_INSN]) = (val))

#define SET_ID_EX_PC(p_reg, val) (((p_reg)->bit_32_regs[ID_EX_PC]) = (val))
#define SET_ID_EX_INSN(p_reg, val) (((p_reg)->bit_32_regs[ID_EX_INSN]) = (val))
#define SET_ID_EX_READ_DATA_1(p_reg, val) (((p_reg)->bit_32_regs[ID_EX_READ_DATA_1]) = (val))
#define SET_ID_EX_READ_DATA_2(p_reg, val) (((p_reg)->bit_32_regs[ID_EX_READ_DATA_2]) = (val))
#define SET_ID_EX_SIGN_EXTEND(p_reg, val) (((p_reg)->bit_32_regs[ID_EX_SIGN_EXTEND]) = (val))
#define SET_ID_EX_WRITE_RT(p_reg, val) (((p_reg)->bit_32_regs[ID_EX_WRITE_RT]) = (val))
#define SET_ID_EX_WRITE_RD(p_reg, val) (((p_reg)->bit_32_regs[ID_EX_WRITE_RD]) = (val))
#define SET_ID_EX_JUMP_ADDR(p_reg, val) (((p_reg)->bit_32_regs[ID_EX_JUMP_ADDR]) = (val))
#define SET_ID_EX_JUMP_TYPE(p_reg, val) (((p_reg)->bit_32_regs[ID_EX_JUMP_TYPE]) = (val))
#define SET_ID_EX_RS(p_reg, val) (((p_reg)->bit_32_regs[ID_EX_RS]) = (val))
#define SET_ID_EX_RT(p_reg, val) (((p_reg)->bit_32_regs[ID_EX_RT]) = (val))

#define SET_EX_MEM_PC_ADD_RESULT(p_reg, val) (((p_reg)->bit_32_regs[EX_MEM_PC_ADD_RESULT]) = (val))
#define SET_EX_MEM_ALU_RESULT(p_reg, val) (((p_reg)->bit_32_regs[EX_MEM_ALU_RESULT]) = (val))
#define SET_EX_MEM_READ_DATA_2(p_reg, val) (((p_reg)->bit_32_regs[EX_MEM_READ_DATA_2]) = (val))
#define SET_EX_MEM_WRITE_TARGET(p_reg, val) (((p_reg)->bit_32_regs[EX_MEM_WRITE_TARGET]) = (val))
#define SET_EX_MEM_JUMP_ADDR(p_reg, val) (((p_reg)->bit_32_regs[EX_MEM_JUMP_ADDR]) = (val))
#define SET_EX_MEM_JUMP_TYPE(p_reg, val) (((p_reg)->bit_32_regs[EX_MEM_JUMP_TYPE]) = (val))
#define SET_EX_MEM_PC(p_reg, val) (((p_reg)->bit_32_regs[EX_MEM_PC]) = (val))
#define SET_EX_MEM_ZERO(p_reg, val) (((p_reg)->bit_8_regs[0]) = (val))

#define SET_MEM_WB_READ_DATA(p_reg, val) (((p_reg)->bit_32_regs[MEM_WB_READ_DATA]) = (val))
#define SET_MEM_WB_ALU_RESULT(p_reg, val) (((p_reg)->bit_32_regs[MEM_WB_ALU_RESULT]) = (val))
#define SET_MEM_WB_WRITE_TARGET(p_reg, val) (((p_reg)->bit_32_regs[MEM_WB_WRITE_TARGET]) = (val))
#define SET_MEM_WB_PC(p_reg, val) (((p_reg)->bit_32_regs[MEM_WB_PC]) = (val))

#define SET_WB_WRITE_TARGET(p_reg, val) (((p_reg)->bit_32_regs[WB_WRITE_TARGET]) = (val))
#define SET_WB_SIG_WRITE_REG(p_reg, val) (((p_reg)->bit_32_regs[WB_SIG_WRITE_REG]) = (val))

#define SET_ID_EX_CTR_SIG(p_reg, val) (((p_reg)->sigs[1]) = (val))
#define SET_EX_MEM_CTR_SIG(p_reg, val) (((p_reg)->sigs[2]) = (val))
#define SET_MEM_WB_CTR_SIG(p_reg, val) (((p_reg)->sigs[3]) = (val))

// Define macros for branch prediction.
#define UPDATE_2_BIT_BRANCH_HISTORY(state, value) \
  do { \
    int temp = (state) + (value); \
    (state) = (temp < 0) ? 0 : (temp > 3) ? 3 : temp; \
  } while (0)
#define PREDICT_2_BIT_BRANCH(state) (((state) > 1)) // 1 if predicted, 0 if not.
#define UPDATE_1_BIT_BRANCH_HISTORY(state, value)((state) = (value == 1))
#define PREDICT_1_BIT_BRANCH(state) ((state) == 1) // 1 if taken, 0 if not.
#define NEED_BRANCH_PREDICTION(val) (GET_OPCODE((val)) == OPCODE_BEQ || GET_OPCODE((val)) == OPCODE_BNE)
// Define macros for string conversion.
#define GET_BP_MODE_STRING(bp_mode) (((bp_mode) == BRANCH_MODE_1_BIT ? \
                            "1 Bit Dynamic" : (bp_mode) == BRANCH_MODE_2_BIT ? "2 Bit Dynamic" : "Static"))
#define BOOL_STRING(value) ((value) ? "true" : "false")
#define DEBUG_PRINT_SIGNALS(signals) \
    printf("    [DEBUG] \n" \
           "   - Signals: RegDst=%0x, AluSrc=%0x, MemToReg=%0x, RegWrite=%0x, MemRead=%0x, MemWrite=%0x, Branch=%0x, AluOp0=%0x, AluOp1=%0x, Jump=%0x\n", \
           (signals)->reg_dst, (signals)->alu_src, (signals)->mem_to_reg, (signals)->reg_write, \
           (signals)->mem_read, (signals)->mem_write, (signals)->branch, (signals)->alu_op_0, (signals)->alu_op_1, (signals)->jump)

// Define macros for last instruction in multi-cycle pipeline.
#define IS_FINISHED(ctx) (((ctx)->cycles_till_terminate) == 0)

// Define macros for jump instruction's type
#define IS_JAL(val) ((val) == JUMP_JAL)
#define IS_JR(val) ((val) == JUMP_JR)

#endif //CA_HW2_UTILS_H
