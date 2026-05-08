#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "linker_script.h"
#include "utils.h"

/*
 * Simple linker script parser.
 * Supports format:
 *
 * MEMORY {
 *     BRAM : ORIGIN = 0x00000000, LENGTH = 8K
 * }
 * SECTIONS {
 *     .text : { *(.text) } > BRAM
 *     .data : { *(.data) } > BRAM
 * }
 * STACK_TOP = 0x00002000;
 */

static bool parse_size(const char *text, uint32_t *out)
{
    char buf[64];
    strncpy(buf, text, sizeof(buf)-1); buf[sizeof(buf)-1]='\0';
    trim(buf);
    size_t len = strlen(buf);
    uint32_t multiplier = 1;
    if (len > 0) {
        char last = buf[len-1];
        if (last == 'K' || last == 'k') { multiplier = 1024; buf[len-1] = '\0'; }
        else if (last == 'M' || last == 'm') { multiplier = 1024*1024; buf[len-1] = '\0'; }
    }
    int64_t val;
    if (!parse_number(buf, &val)) return false;
    *out = (uint32_t)val * multiplier;
    return true;
}

bool parse_linker_script(const char *path, LinkerConfig *config, ErrorList *errors)
{
    FILE *fp = fopen(path, "r");
    if (!fp) {
        errors_add(errors, 0, "Cannot open linker script: %s", path);
        return false;
    }

    linker_config_init(config);

    char line[512];
    uint32_t mem_origin = 0, mem_length = 0x2000;
    bool found_memory = false;

    while (fgets(line, sizeof(line), fp)) {
        /* Strip comments */
        char *comment = strstr(line, "/*");
        if (comment) *comment = '\0';
        comment = strstr(line, "//");
        if (comment) *comment = '\0';
        trim(line);
        if (line[0] == '\0' || line[0] == '{' || line[0] == '}') continue;

        /* STACK_TOP = value; */
        if (strncmp(line, "STACK_TOP", 9) == 0) {
            char *eq = strchr(line, '=');
            if (eq) {
                char val[64];
                strncpy(val, eq+1, sizeof(val)-1); val[sizeof(val)-1]='\0';
                char *semi = strchr(val, ';');
                if (semi) *semi = '\0';
                trim(val);
                int64_t v;
                if (parse_number(val, &v)) config->stack_top = (uint32_t)v;
            }
            continue;
        }

        /* ORIGIN = value */
        char *origin = strstr(line, "ORIGIN");
        if (origin) {
            char *eq = strchr(origin, '=');
            if (eq) {
                char val[64]; strncpy(val, eq+1, sizeof(val)-1); val[sizeof(val)-1]='\0';
                char *comma = strchr(val, ',');
                if (comma) *comma = '\0';
                trim(val);
                int64_t v;
                if (parse_number(val, &v)) { mem_origin = (uint32_t)v; found_memory = true; }
            }
        }

        /* LENGTH = value */
        char *length = strstr(line, "LENGTH");
        if (length) {
            char *eq = strchr(length, '=');
            if (eq) {
                char val[64]; strncpy(val, eq+1, sizeof(val)-1); val[sizeof(val)-1]='\0';
                char *brace = strchr(val, '}');
                if (brace) *brace = '\0';
                trim(val);
                if (!parse_size(val, &mem_length)) {
                    errors_add(errors, 0, "Invalid LENGTH value in linker script");
                }
            }
        }
    }

    fclose(fp);

    if (found_memory) {
        config->text_start = mem_origin;
        config->memory_size = mem_length;
        config->data_start_auto = true;
    }

    printf("Linker script: text=0x%08X mem_size=0x%X stack=0x%08X\n",
           config->text_start, config->memory_size, config->stack_top);
    return true;
}
