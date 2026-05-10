#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stddef.h>

#define MAX_NAME_LEN 128

void trim(char* s);
void trim_leading(char** s);
void copy_cstr(char* dst, size_t dst_size, const char* src);
int parse_reg(const char* s);
int32_t parse_imm_or_label(const char* s, char* label_out, int* is_label);
int parse_mem_operand(const char* s, int32_t* imm_out, int* rs1_out);

#endif
