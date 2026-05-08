#ifndef UTILS_H
#define UTILS_H

#include "common.h"

void errors_add(ErrorList *errors, int line_no, const char *fmt, ...);
void line_vector_init(LineVector *vec);
void line_vector_push(LineVector *vec, const ParsedLine *line);
void line_vector_free(LineVector *vec);
void symbol_table_init(SymbolTable *table);
void symbol_table_free(SymbolTable *table);
Symbol *symbol_table_find(SymbolTable *table, const char *name);
const Symbol *symbol_table_find_const(const SymbolTable *table, const char *name);
bool symbol_table_add(SymbolTable *table, const char *name, uint32_t address, Section section, bool is_absolute);

void memory_image_init(MemoryImage *image, size_t size);
void memory_image_free(MemoryImage *image);
bool memory_write_byte(MemoryImage *image, uint32_t address, uint8_t value);
bool memory_write_word_le(MemoryImage *image, uint32_t address, uint32_t value);

void trim(char *s);
void to_lowercase(char *s);
int split_operands(const char *text, char out[][64], int max_parts);
bool parse_number(const char *text, int64_t *out_value);
int parse_register(const char *text);
bool eval_expr(const char *expr, const SymbolTable *symbols, int64_t *value, bool *is_symbolic);

#endif
