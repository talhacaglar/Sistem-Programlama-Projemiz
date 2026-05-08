#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "hex_writer.h"

static uint8_t checksum_record(uint8_t len, uint16_t addr, uint8_t type, const uint8_t *data)
{
    uint32_t sum = len + (uint8_t)(addr >> 8) + (uint8_t)(addr & 0xFFu) + type;
    for (uint8_t i = 0; i < len; ++i) {
        sum += data[i];
    }
    return (uint8_t)((~sum + 1u) & 0xFFu);
}

static bool write_record(FILE *fp, uint8_t len, uint16_t addr, uint8_t type, const uint8_t *data)
{
    if (fprintf(fp, ":%02X%04X%02X", len, addr, type) < 0) {
        return false;
    }
    for (uint8_t i = 0; i < len; ++i) {
        if (fprintf(fp, "%02X", data[i]) < 0) {
            return false;
        }
    }
    uint8_t csum = checksum_record(len, addr, type, data);
    return fprintf(fp, "%02X\n", csum) > 0;
}

bool write_intel_hex(const char *path, const MemoryImage *image, uint32_t min_used, uint32_t max_used)
{
    FILE *fp = fopen(path, "w");
    if (!fp) {
        return false;
    }

    uint32_t addr = min_used;
    uint16_t current_upper = 0xFFFFu;
    while (addr <= max_used) {
        while (addr <= max_used && !image->used[addr]) {
            ++addr;
        }
        if (addr > max_used) {
            break;
        }

        uint16_t upper = (uint16_t)(addr >> 16);
        if (upper != current_upper) {
            uint8_t ext[2] = {(uint8_t)(upper >> 8), (uint8_t)(upper & 0xFFu)};
            if (!write_record(fp, 2, 0, 0x04, ext)) {
                fclose(fp);
                return false;
            }
            current_upper = upper;
        }

        uint8_t data[16];
        uint8_t len = 0;
        uint16_t low = (uint16_t)(addr & 0xFFFFu);
        while (addr <= max_used && len < 16 && image->used[addr] && ((addr >> 16) == current_upper)) {
            data[len++] = image->data[addr++];
        }
        if (!write_record(fp, len, low, 0x00, data)) {
            fclose(fp);
            return false;
        }
    }

    if (!write_record(fp, 0, 0, 0x01, NULL)) {
        fclose(fp);
        return false;
    }
    fclose(fp);
    return true;
}

bool write_listing(const char *path, const LineVector *lines)
{
    FILE *fp = fopen(path, "w");
    if (!fp) {
        return false;
    }

    fprintf(fp, "Address     Bytes                                Source\n");
    fprintf(fp, "---------------------------------------------------------------\n");

    for (size_t i = 0; i < lines->count; ++i) {
        const ParsedLine *line = &lines->items[i];
        if (line->is_empty) {
            fprintf(fp, "            %-36s %s\n", "", line->original);
            continue;
        }
        fprintf(fp, "%08X    ", line->address);
        if (line->emits_bytes) {
            for (size_t j = 0; j < line->emitted_size; ++j) {
                fprintf(fp, "%02X", line->bytes[j]);
                if (j + 1 < line->emitted_size) {
                    fputc(' ', fp);
                }
            }
        }
        size_t pad = 36;
        size_t used = line->emits_bytes ? line->emitted_size * 3 - 1 : 0;
        if (used < pad) {
            for (size_t k = 0; k < pad - used; ++k) {
                fputc(' ', fp);
            }
        } else {
            fputc(' ', fp);
        }
        fprintf(fp, "%s\n", line->original);
    }

    fclose(fp);
    return true;
}
