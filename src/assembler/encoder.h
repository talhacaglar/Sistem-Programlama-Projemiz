#ifndef ENCODER_H
#define ENCODER_H

#include "assembler.h"

void assemble_instr(Assembler* a, const char* mn, char* args_str, int line_num);

#endif
