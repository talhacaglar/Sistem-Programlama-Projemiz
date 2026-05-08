#include <stdio.h>
#include <string.h>
#include "opcodes.h"

static const OpcodeEntry OPCODES[] = {
    /* R-type */
    {"add",  FMT_R,   OPSTYLE_RD_RS1_RS2, 0x33, 0x0, 0x00},
    {"sub",  FMT_R,   OPSTYLE_RD_RS1_RS2, 0x33, 0x0, 0x20},
    {"sll",  FMT_R,   OPSTYLE_RD_RS1_RS2, 0x33, 0x1, 0x00},
    {"slt",  FMT_R,   OPSTYLE_RD_RS1_RS2, 0x33, 0x2, 0x00},
    {"sltu", FMT_R,   OPSTYLE_RD_RS1_RS2, 0x33, 0x3, 0x00},
    {"xor",  FMT_R,   OPSTYLE_RD_RS1_RS2, 0x33, 0x4, 0x00},
    {"srl",  FMT_R,   OPSTYLE_RD_RS1_RS2, 0x33, 0x5, 0x00},
    {"sra",  FMT_R,   OPSTYLE_RD_RS1_RS2, 0x33, 0x5, 0x20},
    {"or",   FMT_R,   OPSTYLE_RD_RS1_RS2, 0x33, 0x6, 0x00},
    {"and",  FMT_R,   OPSTYLE_RD_RS1_RS2, 0x33, 0x7, 0x00},

    /* I-type ALU */
    {"addi",  FMT_I,  OPSTYLE_RD_RS1_IMM, 0x13, 0x0, 0x00},
    {"slti",  FMT_I,  OPSTYLE_RD_RS1_IMM, 0x13, 0x2, 0x00},
    {"sltiu", FMT_I,  OPSTYLE_RD_RS1_IMM, 0x13, 0x3, 0x00},
    {"xori",  FMT_I,  OPSTYLE_RD_RS1_IMM, 0x13, 0x4, 0x00},
    {"ori",   FMT_I,  OPSTYLE_RD_RS1_IMM, 0x13, 0x6, 0x00},
    {"andi",  FMT_I,  OPSTYLE_RD_RS1_IMM, 0x13, 0x7, 0x00},
    {"slli",  FMT_I,  OPSTYLE_RD_RS1_IMM, 0x13, 0x1, 0x00},
    {"srli",  FMT_I,  OPSTYLE_RD_RS1_IMM, 0x13, 0x5, 0x00},
    {"srai",  FMT_I,  OPSTYLE_RD_RS1_IMM, 0x13, 0x5, 0x20},

    /* loads */
    {"lb",   FMT_I,   OPSTYLE_RD_IMM_RS1, 0x03, 0x0, 0x00},
    {"lh",   FMT_I,   OPSTYLE_RD_IMM_RS1, 0x03, 0x1, 0x00},
    {"lw",   FMT_I,   OPSTYLE_RD_IMM_RS1, 0x03, 0x2, 0x00},
    {"lbu",  FMT_I,   OPSTYLE_RD_IMM_RS1, 0x03, 0x4, 0x00},
    {"lhu",  FMT_I,   OPSTYLE_RD_IMM_RS1, 0x03, 0x5, 0x00},

    /* jalr */
    {"jalr", FMT_I,   OPSTYLE_RD_IMM_RS1, 0x67, 0x0, 0x00},

    /* stores */
    {"sb",   FMT_S,   OPSTYLE_RS2_IMM_RS1, 0x23, 0x0, 0x00},
    {"sh",   FMT_S,   OPSTYLE_RS2_IMM_RS1, 0x23, 0x1, 0x00},
    {"sw",   FMT_S,   OPSTYLE_RS2_IMM_RS1, 0x23, 0x2, 0x00},

    /* branches */
    {"beq",  FMT_B,   OPSTYLE_RS1_RS2_LABEL, 0x63, 0x0, 0x00},
    {"bne",  FMT_B,   OPSTYLE_RS1_RS2_LABEL, 0x63, 0x1, 0x00},
    {"blt",  FMT_B,   OPSTYLE_RS1_RS2_LABEL, 0x63, 0x4, 0x00},
    {"bge",  FMT_B,   OPSTYLE_RS1_RS2_LABEL, 0x63, 0x5, 0x00},
    {"bltu", FMT_B,   OPSTYLE_RS1_RS2_LABEL, 0x63, 0x6, 0x00},
    {"bgeu", FMT_B,   OPSTYLE_RS1_RS2_LABEL, 0x63, 0x7, 0x00},

    /* U/J */
    {"lui",   FMT_U,  OPSTYLE_RD_UIMM,  0x37, 0x0, 0x00},
    {"auipc", FMT_U,  OPSTYLE_RD_UIMM,  0x17, 0x0, 0x00},
    {"jal",   FMT_J,  OPSTYLE_RD_LABEL, 0x6F, 0x0, 0x00},

    /* system */
    {"ecall",  FMT_SYS, OPSTYLE_NO_OPERANDS, 0x73, 0x0, 0x00},
    {"ebreak", FMT_SYS, OPSTYLE_NO_OPERANDS, 0x73, 0x0, 0x01},
};

static const size_t OPCODE_COUNT = sizeof(OPCODES) / sizeof(OPCODES[0]);

const OpcodeEntry *find_opcode(const char *mnemonic)
{
    for (size_t i = 0; i < OPCODE_COUNT; ++i) {
        if (strcmp(OPCODES[i].mnemonic, mnemonic) == 0) {
            return &OPCODES[i];
        }
    }
    return NULL;
}

void print_supported_opcodes(void)
{
    for (size_t i = 0; i < OPCODE_COUNT; ++i) {
        printf("%s\n", OPCODES[i].mnemonic);
    }
}
