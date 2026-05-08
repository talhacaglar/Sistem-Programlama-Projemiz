#ifndef LINKER_H
#define LINKER_H

#include "common.h"

typedef struct {
    uint32_t text_start;    /* .text section start address */
    uint32_t data_start;    /* .data section start address (0 = auto after text) */
    uint32_t stack_top;     /* Stack pointer initial value */
    uint32_t memory_size;   /* Total memory size */
    bool data_start_auto;   /* If true, data follows text */
} LinkerConfig;

/* Initialize linker config with defaults for Tang Nano 9K */
void linker_config_init(LinkerConfig *config);

/* Link multiple object files into a single memory image */
bool linker_link(int obj_count, const char **obj_paths,
                 const LinkerConfig *config,
                 MemoryImage *output,
                 uint32_t *entry_point,
                 uint32_t *min_used,
                 uint32_t *max_used,
                 ErrorList *errors);

#endif
