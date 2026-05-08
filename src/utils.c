#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>
#include "utils.h"

static void *xrealloc(void *ptr, size_t size)
{
    void *p = realloc(ptr, size);
    if (!p) {
        fprintf(stderr, "Out of memory.\n");
        exit(EXIT_FAILURE);
    }
    return p;
}

void errors_add(ErrorList *errors, int line_no, const char *fmt, ...)
{
    if (errors->count >= MAX_ERRORS) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    if (line_no > 0) {
        snprintf(errors->messages[errors->count], sizeof(errors->messages[errors->count]), "Line %d: ", line_no);
        size_t prefix = strlen(errors->messages[errors->count]);
        vsnprintf(errors->messages[errors->count] + prefix,
                  sizeof(errors->messages[errors->count]) - prefix,
                  fmt,
                  ap);
    } else {
        vsnprintf(errors->messages[errors->count], sizeof(errors->messages[errors->count]), fmt, ap);
    }
    va_end(ap);
    errors->count++;
}

void line_vector_init(LineVector *vec)
{
    vec->items = NULL;
    vec->count = 0;
    vec->capacity = 0;
}

void line_vector_push(LineVector *vec, const ParsedLine *line)
{
    if (vec->count == vec->capacity) {
        vec->capacity = vec->capacity ? vec->capacity * 2 : INITIAL_CAPACITY;
        vec->items = xrealloc(vec->items, vec->capacity * sizeof(ParsedLine));
    }
    vec->items[vec->count++] = *line;
}

void line_vector_free(LineVector *vec)
{
    free(vec->items);
    vec->items = NULL;
    vec->count = 0;
    vec->capacity = 0;
}

void symbol_table_init(SymbolTable *table)
{
    table->count = 0;
    table->capacity = 128;
    table->items = calloc(table->capacity, sizeof(Symbol));
    if (!table->items) {
        fprintf(stderr, "Failed to allocate symbol table.\n");
        exit(EXIT_FAILURE);
    }
}

void symbol_table_free(SymbolTable *table)
{
    free(table->items);
    table->items = NULL;
    table->count = 0;
    table->capacity = 0;
}

static uint32_t hash_symbol_name(const char *name)
{
    uint32_t h = 2166136261u;
    while (*name) {
        h ^= (uint8_t)*name++;
        h *= 16777619u;
    }
    return h;
}

static void symbol_table_rehash(SymbolTable *table)
{
    Symbol *old_items = table->items;
    size_t old_cap = table->capacity;

    table->capacity *= 2;
    table->items = calloc(table->capacity, sizeof(Symbol));
    if (!table->items) {
        fprintf(stderr, "Failed to grow symbol table.\n");
        exit(EXIT_FAILURE);
    }
    table->count = 0;

    for (size_t i = 0; i < old_cap; ++i) {
        if (!old_items[i].occupied) {
            continue;
        }
        symbol_table_add(table, old_items[i].name, old_items[i].address, old_items[i].section, old_items[i].is_absolute);
        Symbol *re = symbol_table_find(table, old_items[i].name);
        if (re) {
            re->binding = old_items[i].binding;
            re->defined = old_items[i].defined;
        }
    }
    free(old_items);
}

Symbol *symbol_table_find(SymbolTable *table, const char *name)
{
    if (!table->items || table->capacity == 0) {
        return NULL;
    }
    uint32_t h = hash_symbol_name(name);
    for (size_t probe = 0; probe < table->capacity; ++probe) {
        size_t idx = (h + probe) & (table->capacity - 1);
        Symbol *sym = &table->items[idx];
        if (!sym->occupied) {
            return NULL;
        }
        if (strcmp(sym->name, name) == 0) {
            return sym;
        }
    }
    return NULL;
}

const Symbol *symbol_table_find_const(const SymbolTable *table, const char *name)
{
    if (!table->items || table->capacity == 0) {
        return NULL;
    }
    uint32_t h = hash_symbol_name(name);
    for (size_t probe = 0; probe < table->capacity; ++probe) {
        size_t idx = (h + probe) & (table->capacity - 1);
        const Symbol *sym = &table->items[idx];
        if (!sym->occupied) {
            return NULL;
        }
        if (strcmp(sym->name, name) == 0) {
            return sym;
        }
    }
    return NULL;
}

bool symbol_table_add(SymbolTable *table, const char *name, uint32_t address, Section section, bool is_absolute)
{
    if ((table->count + 1) * 10 >= table->capacity * 7) {
        symbol_table_rehash(table);
    }

    uint32_t h = hash_symbol_name(name);
    for (size_t probe = 0; probe < table->capacity; ++probe) {
        size_t idx = (h + probe) & (table->capacity - 1);
        Symbol *sym = &table->items[idx];
        if (!sym->occupied) {
            memset(sym, 0, sizeof(*sym));
            snprintf(sym->name, sizeof(sym->name), "%s", name);
            sym->address = address;
            sym->section = section;
            sym->defined = true;
            sym->is_absolute = is_absolute;
            sym->occupied = true;
            table->count++;
            return true;
        }
        if (strcmp(sym->name, name) == 0) {
            if (sym->defined) {
                return false;
            }
            sym->address = address;
            sym->section = section;
            sym->defined = true;
            sym->is_absolute = is_absolute;
            return true;
        }
    }
    return false;
}

void memory_image_init(MemoryImage *image, size_t size)
{
    image->size = size;
    image->data = calloc(size, sizeof(uint8_t));
    image->used = calloc(size, sizeof(bool));
    if (!image->data || !image->used) {
        fprintf(stderr, "Failed to allocate memory image.\n");
        exit(EXIT_FAILURE);
    }
}

void memory_image_free(MemoryImage *image)
{
    free(image->data);
    free(image->used);
    image->data = NULL;
    image->used = NULL;
    image->size = 0;
}

bool memory_write_byte(MemoryImage *image, uint32_t address, uint8_t value)
{
    if ((size_t)address >= image->size) {
        return false;
    }
    image->data[address] = value;
    image->used[address] = true;
    return true;
}

bool memory_write_word_le(MemoryImage *image, uint32_t address, uint32_t value)
{
    return memory_write_byte(image, address + 0u, (uint8_t)(value & 0xFFu)) &&
           memory_write_byte(image, address + 1u, (uint8_t)((value >> 8) & 0xFFu)) &&
           memory_write_byte(image, address + 2u, (uint8_t)((value >> 16) & 0xFFu)) &&
           memory_write_byte(image, address + 3u, (uint8_t)((value >> 24) & 0xFFu));
}

void trim(char *s)
{
    if (!s || !*s) {
        return;
    }
    char *start = s;
    while (*start && isspace((unsigned char)*start)) {
        ++start;
    }
    if (start != s) {
        memmove(s, start, strlen(start) + 1u);
    }
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
}

void to_lowercase(char *s)
{
    while (*s) {
        *s = (char)tolower((unsigned char)*s);
        ++s;
    }
}

int split_operands(const char *text, char out[][64], int max_parts)
{
    int count = 0;
    const char *p = text;
    while (*p && count < max_parts) {
        while (*p && isspace((unsigned char)*p)) {
            ++p;
        }
        if (!*p) {
            break;
        }
        int depth = 0;
        char buf[64] = {0};
        size_t pos = 0;
        while (*p) {
            if (*p == '(') depth++;
            if (*p == ')') depth--;
            if (*p == ',' && depth == 0) {
                ++p;
                break;
            }
            if (pos + 1 < sizeof(buf)) {
                buf[pos++] = *p;
            }
            ++p;
        }
        buf[pos] = '\0';
        trim(buf);
        snprintf(out[count], 64, "%s", buf);
        count++;
    }
    return count;
}

bool parse_number(const char *text, int64_t *out_value)
{
    if (!text || !*text) {
        return false;
    }
    char *end = NULL;
    errno = 0;
    int base = 10;
    const char *numstart = text;
    if (strlen(text) > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        base = 16;
    } else if (strlen(text) > 2 && text[0] == '0' && (text[1] == 'b' || text[1] == 'B')) {
        base = 2;
        numstart = text + 2;
    }
    long long value = strtoll(numstart, &end, base);
    if (errno != 0 || end == numstart || *end != '\0') {
        return false;
    }
    *out_value = (int64_t)value;
    return true;
}

static bool parse_char_literal(const char *text, int64_t *out_value)
{
    size_t len = strlen(text);
    if (len >= 3 && text[0] == '\'' && text[len - 1] == '\'') {
        if (len == 3) {
            *out_value = (unsigned char)text[1];
            return true;
        }
        if (len == 4 && text[1] == '\\') {
            switch (text[2]) {
                case 'n': *out_value = '\n'; return true;
                case 'r': *out_value = '\r'; return true;
                case 't': *out_value = '\t'; return true;
                case '\\': *out_value = '\\'; return true;
                case '\'': *out_value = '\''; return true;
                default: return false;
            }
        }
    }
    return false;
}

int parse_register(const char *text)
{
    static const struct { const char *name; int idx; } aliases[] = {
        {"zero", 0}, {"ra", 1}, {"sp", 2}, {"gp", 3}, {"tp", 4},
        {"t0", 5}, {"t1", 6}, {"t2", 7}, {"s0", 8}, {"fp", 8}, {"s1", 9},
        {"a0", 10}, {"a1", 11}, {"a2", 12}, {"a3", 13}, {"a4", 14}, {"a5", 15},
        {"a6", 16}, {"a7", 17}, {"s2", 18}, {"s3", 19}, {"s4", 20}, {"s5", 21},
        {"s6", 22}, {"s7", 23}, {"s8", 24}, {"s9", 25}, {"s10", 26}, {"s11", 27},
        {"t3", 28}, {"t4", 29}, {"t5", 30}, {"t6", 31}
    };

    if (!text || !*text) {
        return -1;
    }

    if (text[0] == 'x') {
        int64_t val = 0;
        if (parse_number(text + 1, &val) && val >= 0 && val <= 31) {
            return (int)val;
        }
    }

    for (size_t i = 0; i < sizeof(aliases) / sizeof(aliases[0]); ++i) {
        if (strcmp(text, aliases[i].name) == 0) {
            return aliases[i].idx;
        }
    }
    return -1;
}

bool eval_expr(const char *expr, const SymbolTable *symbols, int64_t *value, bool *is_symbolic)
{
    char buf[128];
    strncpy(buf, expr, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    trim(buf);

    int64_t num = 0;
    if (parse_number(buf, &num) || parse_char_literal(buf, &num)) {
        *value = num;
        *is_symbolic = false;
        return true;
    }

    const char *plus = strchr(buf, '+');
    const char *minus = NULL;
    if (!plus) {
        minus = strrchr(buf + 1, '-');
    }

    if (!plus && !minus) {
        const Symbol *sym = symbol_table_find_const(symbols, buf);
        if (!sym || !sym->defined) {
            return false;
        }
        *value = (int64_t)sym->address;
        *is_symbolic = true;
        return true;
    }

    char left[64] = {0};
    char right[64] = {0};
    bool subtract = minus != NULL;
    const char *op = plus ? plus : minus;
    strncpy(left, buf, (size_t)(op - buf));
    strncpy(right, op + 1, sizeof(right) - 1);
    trim(left);
    trim(right);

    const Symbol *sym = symbol_table_find_const(symbols, left);
    if (!sym || !sym->defined) {
        return false;
    }
    int64_t rhs = 0;
    if (!(parse_number(right, &rhs) || parse_char_literal(right, &rhs))) {
        return false;
    }
    *value = subtract ? ((int64_t)sym->address - rhs) : ((int64_t)sym->address + rhs);
    *is_symbolic = true;
    return true;
}
