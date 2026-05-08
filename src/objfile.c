#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "objfile.h"
#include "utils.h"

/* ------------------------------------------------------------------ */
/*  Relocation list helpers                                           */
/* ------------------------------------------------------------------ */

void reloc_list_init(RelocationList *rl)
{
    rl->items = NULL;
    rl->count = 0;
    rl->capacity = 0;
}

void reloc_list_push(RelocationList *rl, const RelocationEntry *entry)
{
    if (rl->count == rl->capacity) {
        rl->capacity = rl->capacity ? rl->capacity * 2 : 16;
        rl->items = realloc(rl->items, rl->capacity * sizeof(RelocationEntry));
        if (!rl->items) {
            fprintf(stderr, "Out of memory (reloc list).\n");
            exit(EXIT_FAILURE);
        }
    }
    rl->items[rl->count++] = *entry;
}

void reloc_list_free(RelocationList *rl)
{
    free(rl->items);
    rl->items = NULL;
    rl->count = 0;
    rl->capacity = 0;
}

/* ------------------------------------------------------------------ */
/*  ObjectFile init / free                                            */
/* ------------------------------------------------------------------ */

void object_file_init(ObjectFile *obj)
{
    memset(obj, 0, sizeof(*obj));
    snprintf(obj->text.name, sizeof(obj->text.name), ".text");
    snprintf(obj->data.name, sizeof(obj->data.name), ".data");
    symbol_table_init(&obj->symbols);
    reloc_list_init(&obj->text_relocs);
    reloc_list_init(&obj->data_relocs);
}

void object_file_free(ObjectFile *obj)
{
    free(obj->text.data);
    free(obj->data.data);
    symbol_table_free(&obj->symbols);
    reloc_list_free(&obj->text_relocs);
    reloc_list_free(&obj->data_relocs);
    memset(obj, 0, sizeof(*obj));
}

/* ------------------------------------------------------------------ */
/*  Helper: write/read exact bytes                                    */
/* ------------------------------------------------------------------ */

static bool write_u32(FILE *fp, uint32_t v)
{
    uint8_t buf[4];
    buf[0] = (uint8_t)(v & 0xFF);
    buf[1] = (uint8_t)((v >> 8) & 0xFF);
    buf[2] = (uint8_t)((v >> 16) & 0xFF);
    buf[3] = (uint8_t)((v >> 24) & 0xFF);
    return fwrite(buf, 1, 4, fp) == 4;
}

static bool read_u32(FILE *fp, uint32_t *v)
{
    uint8_t buf[4];
    if (fread(buf, 1, 4, fp) != 4) return false;
    *v = (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) |
         ((uint32_t)buf[2] << 16) | ((uint32_t)buf[3] << 24);
    return true;
}

static bool write_i64(FILE *fp, int64_t v)
{
    uint8_t buf[8];
    uint64_t u = (uint64_t)v;
    for (int i = 0; i < 8; i++) {
        buf[i] = (uint8_t)(u & 0xFF);
        u >>= 8;
    }
    return fwrite(buf, 1, 8, fp) == 8;
}

static bool read_i64(FILE *fp, int64_t *v)
{
    uint8_t buf[8];
    if (fread(buf, 1, 8, fp) != 8) return false;
    uint64_t u = 0;
    for (int i = 7; i >= 0; i--) {
        u = (u << 8) | buf[i];
    }
    *v = (int64_t)u;
    return true;
}

static bool write_str(FILE *fp, const char *s, size_t fixed_len)
{
    char buf[256];
    memset(buf, 0, sizeof(buf));
    size_t len = fixed_len < sizeof(buf) ? fixed_len : sizeof(buf);
    strncpy(buf, s, len);
    return fwrite(buf, 1, len, fp) == len;
}

static bool read_str(FILE *fp, char *s, size_t fixed_len)
{
    if (fread(s, 1, fixed_len, fp) != fixed_len) return false;
    s[fixed_len - 1] = '\0';
    return true;
}

/* ------------------------------------------------------------------ */
/*  Write object file                                                 */
/* ------------------------------------------------------------------ */

bool write_object_file(const char *path, const ObjectFile *obj)
{
    FILE *fp = fopen(path, "wb");
    if (!fp) return false;

    /* Magic */
    if (fwrite(OBJ_MAGIC, 1, OBJ_MAGIC_LEN, fp) != OBJ_MAGIC_LEN) goto fail;

    /* Version */
    if (!write_u32(fp, OBJ_VERSION)) goto fail;

    /* Source name (256 bytes fixed) */
    if (!write_str(fp, obj->source_name, 256)) goto fail;

    /* Section headers: text */
    if (!write_u32(fp, obj->text.base_address)) goto fail;
    if (!write_u32(fp, obj->text.size)) goto fail;

    /* Section headers: data */
    if (!write_u32(fp, obj->data.base_address)) goto fail;
    if (!write_u32(fp, obj->data.size)) goto fail;

    /* Symbol table */
    /* Count only occupied symbols */
    uint32_t sym_count = 0;
    for (size_t i = 0; i < obj->symbols.capacity; i++) {
        if (obj->symbols.items[i].occupied) sym_count++;
    }
    if (!write_u32(fp, sym_count)) goto fail;

    for (size_t i = 0; i < obj->symbols.capacity; i++) {
        const Symbol *sym = &obj->symbols.items[i];
        if (!sym->occupied) continue;
        if (!write_str(fp, sym->name, MAX_LABEL_LEN + 1)) goto fail;
        if (!write_u32(fp, sym->address)) goto fail;
        if (!write_u32(fp, (uint32_t)sym->section)) goto fail;
        if (!write_u32(fp, (uint32_t)sym->binding)) goto fail;
        if (!write_u32(fp, sym->defined ? 1 : 0)) goto fail;
        if (!write_u32(fp, sym->is_absolute ? 1 : 0)) goto fail;
    }

    /* Text relocations */
    if (!write_u32(fp, (uint32_t)obj->text_relocs.count)) goto fail;
    for (size_t i = 0; i < obj->text_relocs.count; i++) {
        const RelocationEntry *r = &obj->text_relocs.items[i];
        if (!write_u32(fp, r->offset)) goto fail;
        if (!write_u32(fp, (uint32_t)r->type)) goto fail;
        if (!write_str(fp, r->symbol_name, MAX_LABEL_LEN + 1)) goto fail;
        if (!write_i64(fp, r->addend)) goto fail;
    }

    /* Data relocations */
    if (!write_u32(fp, (uint32_t)obj->data_relocs.count)) goto fail;
    for (size_t i = 0; i < obj->data_relocs.count; i++) {
        const RelocationEntry *r = &obj->data_relocs.items[i];
        if (!write_u32(fp, r->offset)) goto fail;
        if (!write_u32(fp, (uint32_t)r->type)) goto fail;
        if (!write_str(fp, r->symbol_name, MAX_LABEL_LEN + 1)) goto fail;
        if (!write_i64(fp, r->addend)) goto fail;
    }

    /* Text section data */
    if (obj->text.size > 0) {
        if (fwrite(obj->text.data, 1, obj->text.size, fp) != obj->text.size) goto fail;
    }

    /* Data section data */
    if (obj->data.size > 0) {
        if (fwrite(obj->data.data, 1, obj->data.size, fp) != obj->data.size) goto fail;
    }

    fclose(fp);
    return true;

fail:
    fclose(fp);
    return false;
}

/* ------------------------------------------------------------------ */
/*  Read object file                                                  */
/* ------------------------------------------------------------------ */

bool read_object_file(const char *path, ObjectFile *obj)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) return false;

    object_file_init(obj);

    /* Magic */
    char magic[OBJ_MAGIC_LEN];
    if (fread(magic, 1, OBJ_MAGIC_LEN, fp) != OBJ_MAGIC_LEN) goto fail;
    if (memcmp(magic, OBJ_MAGIC, OBJ_MAGIC_LEN) != 0) {
        fprintf(stderr, "Error: %s is not a valid RV32 object file.\n", path);
        goto fail;
    }

    /* Version */
    uint32_t version;
    if (!read_u32(fp, &version)) goto fail;
    if (version != OBJ_VERSION) {
        fprintf(stderr, "Error: unsupported object file version %u.\n", version);
        goto fail;
    }

    /* Source name */
    if (!read_str(fp, obj->source_name, 256)) goto fail;

    /* Section headers */
    if (!read_u32(fp, &obj->text.base_address)) goto fail;
    if (!read_u32(fp, &obj->text.size)) goto fail;
    if (!read_u32(fp, &obj->data.base_address)) goto fail;
    if (!read_u32(fp, &obj->data.size)) goto fail;

    /* Symbol table */
    uint32_t sym_count;
    if (!read_u32(fp, &sym_count)) goto fail;

    for (uint32_t i = 0; i < sym_count; i++) {
        char name[MAX_LABEL_LEN + 1];
        uint32_t address, sec_val, bind_val, def_val, abs_val;

        if (!read_str(fp, name, MAX_LABEL_LEN + 1)) goto fail;
        if (!read_u32(fp, &address)) goto fail;
        if (!read_u32(fp, &sec_val)) goto fail;
        if (!read_u32(fp, &bind_val)) goto fail;
        if (!read_u32(fp, &def_val)) goto fail;
        if (!read_u32(fp, &abs_val)) goto fail;

        /* Add to symbol table - for extern (undefined) symbols, add with special handling */
        if (bind_val == BIND_EXTERN && def_val == 0) {
            /* Add as undefined extern */
            symbol_table_add(&obj->symbols, name, 0, SEC_TEXT, false);
            Symbol *sym = symbol_table_find(&obj->symbols, name);
            if (sym) {
                sym->binding = (SymbolBinding)bind_val;
                sym->defined = false;
            }
        } else {
            symbol_table_add(&obj->symbols, name, address, (Section)sec_val, abs_val != 0);
            Symbol *sym = symbol_table_find(&obj->symbols, name);
            if (sym) {
                sym->binding = (SymbolBinding)bind_val;
                sym->defined = (def_val != 0);
            }
        }
    }

    /* Text relocations */
    uint32_t text_reloc_count;
    if (!read_u32(fp, &text_reloc_count)) goto fail;
    for (uint32_t i = 0; i < text_reloc_count; i++) {
        RelocationEntry r;
        uint32_t type_val;
        if (!read_u32(fp, &r.offset)) goto fail;
        if (!read_u32(fp, &type_val)) goto fail;
        r.type = (RelocationType)type_val;
        if (!read_str(fp, r.symbol_name, MAX_LABEL_LEN + 1)) goto fail;
        if (!read_i64(fp, &r.addend)) goto fail;
        reloc_list_push(&obj->text_relocs, &r);
    }

    /* Data relocations */
    uint32_t data_reloc_count;
    if (!read_u32(fp, &data_reloc_count)) goto fail;
    for (uint32_t i = 0; i < data_reloc_count; i++) {
        RelocationEntry r;
        uint32_t type_val;
        if (!read_u32(fp, &r.offset)) goto fail;
        if (!read_u32(fp, &type_val)) goto fail;
        r.type = (RelocationType)type_val;
        if (!read_str(fp, r.symbol_name, MAX_LABEL_LEN + 1)) goto fail;
        if (!read_i64(fp, &r.addend)) goto fail;
        reloc_list_push(&obj->data_relocs, &r);
    }

    /* Text section data */
    if (obj->text.size > 0) {
        obj->text.data = malloc(obj->text.size);
        if (!obj->text.data) goto fail;
        if (fread(obj->text.data, 1, obj->text.size, fp) != obj->text.size) goto fail;
    }

    /* Data section data */
    if (obj->data.size > 0) {
        obj->data.data = malloc(obj->data.size);
        if (!obj->data.data) goto fail;
        if (fread(obj->data.data, 1, obj->data.size, fp) != obj->data.size) goto fail;
    }

    fclose(fp);
    return true;

fail:
    fclose(fp);
    object_file_free(obj);
    return false;
}

/* ------------------------------------------------------------------ */
/*  Dump object file (debug utility)                                  */
/* ------------------------------------------------------------------ */

void dump_object_file(const ObjectFile *obj)
{
    printf("=== Object File: %s ===\n", obj->source_name);
    printf("\n--- Sections ---\n");
    printf("  .text : base=0x%08X  size=%u bytes\n", obj->text.base_address, obj->text.size);
    printf("  .data : base=0x%08X  size=%u bytes\n", obj->data.base_address, obj->data.size);

    printf("\n--- Symbol Table ---\n");
    printf("  %-20s %-10s %-8s %-8s %-8s\n", "Name", "Address", "Section", "Binding", "Defined");
    for (size_t i = 0; i < obj->symbols.capacity; i++) {
        const Symbol *sym = &obj->symbols.items[i];
        if (!sym->occupied) continue;
        const char *sec_str = (sym->section == SEC_TEXT) ? ".text" : ".data";
        const char *bind_str = (sym->binding == BIND_LOCAL) ? "LOCAL" :
                               (sym->binding == BIND_GLOBAL) ? "GLOBAL" : "EXTERN";
        printf("  %-20s 0x%08X %-8s %-8s %-8s\n",
               sym->name, sym->address, sec_str, bind_str,
               sym->defined ? "yes" : "no");
    }

    printf("\n--- Text Relocations (%zu) ---\n", obj->text_relocs.count);
    for (size_t i = 0; i < obj->text_relocs.count; i++) {
        const RelocationEntry *r = &obj->text_relocs.items[i];
        printf("  offset=0x%04X type=%d sym=%s addend=%lld\n",
               r->offset, (int)r->type, r->symbol_name, (long long)r->addend);
    }

    printf("\n--- Data Relocations (%zu) ---\n", obj->data_relocs.count);
    for (size_t i = 0; i < obj->data_relocs.count; i++) {
        const RelocationEntry *r = &obj->data_relocs.items[i];
        printf("  offset=0x%04X type=%d sym=%s addend=%lld\n",
               r->offset, (int)r->type, r->symbol_name, (long long)r->addend);
    }

    /* Hex dump of text section */
    if (obj->text.size > 0) {
        printf("\n--- Text Section Hex Dump ---\n");
        for (uint32_t i = 0; i < obj->text.size; i += 4) {
            if (i + 3 < obj->text.size) {
                uint32_t word = (uint32_t)obj->text.data[i] |
                                ((uint32_t)obj->text.data[i+1] << 8) |
                                ((uint32_t)obj->text.data[i+2] << 16) |
                                ((uint32_t)obj->text.data[i+3] << 24);
                printf("  %04X: %08X\n", i, word);
            }
        }
    }
}
