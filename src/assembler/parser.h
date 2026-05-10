#ifndef PARSER_H
#define PARSER_H

#include "assembler.h"

void emit_word(Assembler* a, uint32_t w);
void emit_data_byte(Assembler* a, uint8_t b);
void emit_data_word(Assembler* a, uint32_t w);
void add_reloc(Assembler* a, uint32_t offset, const char* section, RelocType type, const char* sym, int32_t addend);
void add_symbol(Assembler* a, const char* name, const char* section, uint32_t offset, int is_global);
void add_label_fp(Assembler* a, const char* name, const char* section, uint32_t off);
Symbol* find_label(Assembler* a, const char* name);
int is_global_sym(Assembler* a, const char* name);

void process_directive(Assembler* a, const char* dir, char* args);
void first_pass(Assembler* a, char lines[][MAX_LINE_LEN], int line_count);
void second_pass(Assembler* a, char lines[][MAX_LINE_LEN], int line_count);
void resolve_local_relocs(Assembler* a);

#endif
