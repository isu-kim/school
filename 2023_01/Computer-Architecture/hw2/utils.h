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

// Macros for future use :)
// For converting big endian to little endian.
#define BIG_TO_LITTLE_32(x) (((x) >> 24) | (((x) >> 8) & 0xff00) | (((x) << 8) & 0xff0000) | ((x) << 24))
// For converting pc address to the memory's index.
#define CONVERT_PC(x) ((x) / 4) // Kind of not sure whether to deduct 0x10000000.
// For getting opcode from the instruction.
#define GET_OPCODE(x) (((x) & 0xFC000000) >> 26)
// For getting funct code from the instruction.
#define GET_FUNCT(x) (((x) & 0x3F))
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

#endif //CA_HW2_UTILS_H
