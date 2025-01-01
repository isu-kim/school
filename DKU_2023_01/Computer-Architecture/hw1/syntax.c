//
// @file : syntax.c
// @author : Isu Kim @ isu@isu.kim
// @brief : A file that implements functions for simple assembly syntax checker.
//

#include "syntax.h"

extern struct instruction_t *all_instructions;

/**
 * A function that checks single line's syntax.
 * @param line The line to check syntax.
 * @param err The string for storing error message into.
 * @param user_instruction The pointer to user_instruction_t to store parsed instruction into.
 * @return 0 if valid, -1 if invalid.
 */
int parse_line(const char* line, char *err, struct user_instruction_t *user_instruction) {
    int idx = 0;
    uint8_t token_count = 3;
    char* ptr;
    struct user_instruction_t instruction = {
            .instruction = 0,
            .param_count = 0
    };

    char base[MAX_STRING] = {0};
    strcpy(base, line);

    // Remove # comments.
    for (uint32_t i = 0 ; i < MAX_STRING ; i++) {
        if (base[i] == '#') base[i] = 0;
    }

    // Split by whitespaces.
    for (ptr = strtok(base, " ") ; ptr != NULL && idx <= token_count ; ptr = strtok(NULL, " "), idx++) {
#ifdef DEBUG
        printf("%d : \"%s\"\t", idx, ptr);
#endif
        switch (idx) {
            case 0: { // Parse instruction name.
                char instruction_string[5];
                strcpy(instruction_string, ptr);
                TO_LOWER(instruction_string);
                instruction.instruction = translate_string(instruction_string);

                // Check error when parsing instruction string.
                if (instruction.instruction == UNKNOWN) {
                    sprintf(err, "unknown instruction: %s", instruction_string);
                    return -1;
                }

                token_count = all_instructions[instruction.instruction].token_count;
                break;
            }

            case 1: // Parse parameters.
            case 2:
            case 3: {
                // When there are redundant arguments.
                if (idx > all_instructions[instruction.instruction].token_count) {
                    sprintf(err, "redundant argument: %s, %d", ptr, instruction.instruction);
                    return -2;
                }

                char param[MAX_ARG_STRING];
                strcpy(param, ptr);
                TO_LOWER(param);

                // Parse argument.
                struct param_t parsed;
                if (parse_param(param, &parsed, err) != 0) {
                    return -1;
                }

                // If argument was parsable, check syntax.
                if ((all_instructions[instruction.instruction].syntax[idx - 1] == Both) ||
                        (all_instructions[instruction.instruction].syntax[idx - 1] == parsed.type)) {
                    instruction.param[idx - 1] = parsed;
                    instruction.param_count++;
                    continue;
                } else {
                    char expected[10];
                    char got[10];
                    // Show why the syntax was invalid.
                    if (all_instructions[instruction.instruction].syntax[idx - 1] == Register) {
                        strcpy(expected, "register");
                        strcpy(got, "constant");
                    } else {
                        strcpy(expected, "constant");
                        strcpy(got, "register");
                    }

                    sprintf(err, "invalid argument type: %s\n\texpected %s but got %s", param, expected, got);
                    return -1;
                }
            }

            default:
                break;
        }
    }

#ifdef DEBUG
    printf("[DEBUG] %s: %d, %d, %d\n", all_instructions[instruction.instruction].name, instruction.param[0].value,
           instruction.param[1].value, instruction.param[2].value);
#endif

    *user_instruction = instruction;
    return 0;
}

/**
 * A function that parses a string into uint32_t param.
 * @param param The string to translate into param.
 * @param ret The pointer to struct param_t to store translated parameter.
 * @param err The string to store error message into.
 * @return 0 if successful, -1 if not.
 */
int parse_param(const char *param, struct param_t *ret, char *err) {
    if (param[0] == 'r') { // For registers
        char *end;
        ret->type = Register;
        ret->value = strtol(param + 1, &end, 10);

        // If register was invalid.
        if (errno == ERANGE || *end != '\0' || ret->value < 0 || ret->value > 10) {
            sprintf(err, "unknown register: %s", param);
            return -1;
        }
        return 0;
    } else { // For constants
        char *end;
        ret->type = Constant;
        ret->value = strtol(param, &end, 16);

        // If constant was invalid.
        if (errno == ERANGE || *end != '\0') {
            sprintf(err, "invalid constant value: %s", param);
            return -1;
        }
        return 0;
    }
}

/**
 * A function that matches string into instruction enum.
 * @param instruction The string instruction to translate into enum.
 * @return The enum value for instruction. -1 if unknown instruction.
 */
uint8_t translate_string(const char *instruction) {
    if (strcmp(instruction, "add") == 0) {
        return Add;
    } else if (strcmp(instruction, "sub") == 0) {
        return Sub;
    } else if (strcmp(instruction, "mul") == 0) {
        return Mul;
    } else if (strcmp(instruction, "div") == 0) {
        return Div;
    } else if (strcmp(instruction, "mov") == 0) {
        return Mov;
    } else if (strcmp(instruction, "lw") == 0) {
        return Lw;
    } else if (strcmp(instruction, "sw") == 0) {
        return Sw;
    } else if (strcmp(instruction, "rst") == 0) {
        return Rst;
    } else if (strcmp(instruction, "jmp") == 0) {
        return Jmp;
    } else if (strcmp(instruction, "beq") == 0) {
        return Beq;
    } else if (strcmp(instruction, "bne") == 0) {
        return Bne;
    } else if (strcmp(instruction, "slt") == 0) {
        return Slt;
    }  else {
        return UNKNOWN;
    }
}