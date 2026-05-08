#ifndef HEX_WRITER_H
#define HEX_WRITER_H

#include "common.h"

bool write_intel_hex(const char *path, const MemoryImage *image, uint32_t min_used, uint32_t max_used);
bool write_listing(const char *path, const LineVector *lines);

#endif
