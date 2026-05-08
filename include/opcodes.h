#ifndef OPCODES_H
#define OPCODES_H

#include "common.h"

typedef struct {
    const char *mnemonic;
    InstrFormat format;
    OperandStyle style;
    uint8_t opcode;
    uint8_t funct3;
    uint8_t funct7;
} OpcodeEntry;

const OpcodeEntry *find_opcode(const char *mnemonic);
void print_supported_opcodes(void);

#endif
