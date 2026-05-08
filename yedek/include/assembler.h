#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "common.h"

bool run_pass1(LineVector *lines, SymbolTable *symbols, ErrorList *errors, uint32_t *program_end);
bool run_pass2(LineVector *lines,
               const SymbolTable *symbols,
               MemoryImage *image,
               ErrorList *errors,
               uint32_t *entry_point,
               uint32_t *min_used,
               uint32_t *max_used);

/* Assemble source lines into an ObjectFile (for linker workflow) */
bool assemble_to_object(LineVector *lines, SymbolTable *symbols, ObjectFile *obj, ErrorList *errors);

#endif
