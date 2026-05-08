#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linker.h"
#include "objfile.h"
#include "utils.h"

void linker_config_init(LinkerConfig *config)
{
    config->text_start = 0x00000000;
    config->data_start = 0;
    config->stack_top  = 0x00002000;  /* 8KB BRAM top for Tang Nano 9K */
    config->memory_size = 0x00002000; /* 8KB */
    config->data_start_auto = true;
}

/* ------------------------------------------------------------------ */
/*  Internal: merged symbol entry for global resolution               */
/* ------------------------------------------------------------------ */
typedef struct {
    char name[MAX_LABEL_LEN + 1];
    uint32_t final_address;
    int  defining_obj;    /* index of the object that defines it, -1 if undef */
    bool defined;
    Section section;
} GlobalSymbol;

typedef struct {
    GlobalSymbol *items;
    size_t count;
    size_t capacity;
} GlobalSymbolTable;

static void gst_init(GlobalSymbolTable *g) { g->items=NULL; g->count=0; g->capacity=0; }
static void gst_free(GlobalSymbolTable *g) { free(g->items); g->items=NULL; g->count=0; }

static GlobalSymbol *gst_find(GlobalSymbolTable *g, const char *name) {
    for (size_t i=0; i<g->count; i++)
        if (strcmp(g->items[i].name, name)==0) return &g->items[i];
    return NULL;
}

static GlobalSymbol *gst_add(GlobalSymbolTable *g, const char *name) {
    if (g->count == g->capacity) {
        g->capacity = g->capacity ? g->capacity*2 : 64;
        g->items = realloc(g->items, g->capacity * sizeof(GlobalSymbol));
    }
    GlobalSymbol *s = &g->items[g->count++];
    memset(s, 0, sizeof(*s));
    strncpy(s->name, name, MAX_LABEL_LEN);
    s->defining_obj = -1;
    return s;
}

/* ------------------------------------------------------------------ */
/*  Apply a single relocation to a section buffer                     */
/* ------------------------------------------------------------------ */
static bool apply_relocation(uint8_t *section_data, uint32_t section_size,
                             const RelocationEntry *rel, uint32_t symbol_addr,
                             uint32_t reloc_abs_addr, ErrorList *errors)
{
    uint32_t off = rel->offset;
    if (off + 4 > section_size) {
        errors_add(errors, 0, "Relocation offset 0x%X out of bounds.", off);
        return false;
    }

    uint32_t target = (uint32_t)((int64_t)symbol_addr + rel->addend);
    uint32_t instr = (uint32_t)section_data[off] |
                     ((uint32_t)section_data[off+1]<<8) |
                     ((uint32_t)section_data[off+2]<<16) |
                     ((uint32_t)section_data[off+3]<<24);

    switch (rel->type) {
    case RELOC_R_RISCV_32: {
        /* Absolute 32-bit (.word) */
        uint32_t val = target;
        section_data[off+0]=(uint8_t)(val&0xFF);
        section_data[off+1]=(uint8_t)((val>>8)&0xFF);
        section_data[off+2]=(uint8_t)((val>>16)&0xFF);
        section_data[off+3]=(uint8_t)((val>>24)&0xFF);
        break;
    }
    case RELOC_R_RISCV_JAL: {
        /* J-type: PC-relative */
        int32_t offset = (int32_t)(target - reloc_abs_addr);
        uint32_t imm = (uint32_t)offset & 0x1FFFFFu;
        instr &= 0xFFFu; /* keep opcode + rd */
        instr |= (((imm>>20)&1u)<<31) | (((imm>>1)&0x3FFu)<<21) |
                 (((imm>>11)&1u)<<20) | (((imm>>12)&0xFFu)<<12);
        section_data[off+0]=(uint8_t)(instr&0xFF);
        section_data[off+1]=(uint8_t)((instr>>8)&0xFF);
        section_data[off+2]=(uint8_t)((instr>>16)&0xFF);
        section_data[off+3]=(uint8_t)((instr>>24)&0xFF);
        break;
    }
    case RELOC_R_RISCV_BRANCH: {
        /* B-type: PC-relative */
        int32_t offset = (int32_t)(target - reloc_abs_addr);
        uint32_t imm = (uint32_t)offset & 0x1FFFu;
        instr &= 0x01FFF07Fu; /* keep opcode, funct3, rs1, rs2 */
        /* Actually need to keep rs1, rs2, funct3 which are bits [14:12],[19:15],[24:20] */
        uint32_t base = instr & ((0x1Fu<<20)|(0x1Fu<<15)|(0x7u<<12)|0x7Fu);
        instr = base |
                (((imm>>12)&1u)<<31) | (((imm>>5)&0x3Fu)<<25) |
                (((imm>>1)&0xFu)<<8) | (((imm>>11)&1u)<<7);
        section_data[off+0]=(uint8_t)(instr&0xFF);
        section_data[off+1]=(uint8_t)((instr>>8)&0xFF);
        section_data[off+2]=(uint8_t)((instr>>16)&0xFF);
        section_data[off+3]=(uint8_t)((instr>>24)&0xFF);
        break;
    }
    case RELOC_R_RISCV_HI20: {
        /* U-type: upper 20 bits of absolute address */
        uint32_t hi = (target + 0x800) & 0xFFFFF000u;
        instr = (instr & 0xFFFu) | hi;
        section_data[off+0]=(uint8_t)(instr&0xFF);
        section_data[off+1]=(uint8_t)((instr>>8)&0xFF);
        section_data[off+2]=(uint8_t)((instr>>16)&0xFF);
        section_data[off+3]=(uint8_t)((instr>>24)&0xFF);
        break;
    }
    case RELOC_R_RISCV_LO12_I: {
        /* I-type: lower 12 bits */
        uint32_t lo = target & 0xFFFu;
        instr = (instr & 0x000FFFFFu) | (lo << 20);
        section_data[off+0]=(uint8_t)(instr&0xFF);
        section_data[off+1]=(uint8_t)((instr>>8)&0xFF);
        section_data[off+2]=(uint8_t)((instr>>16)&0xFF);
        section_data[off+3]=(uint8_t)((instr>>24)&0xFF);
        break;
    }
    case RELOC_R_RISCV_LO12_S: {
        /* S-type: lower 12 bits split into imm[11:5] and imm[4:0] */
        uint32_t lo = target & 0xFFFu;
        instr = instr & ((0x1Fu<<20)|(0x1Fu<<15)|(0x7u<<12)|0x7Fu);
        instr |= (((lo>>5)&0x7Fu)<<25) | ((lo&0x1Fu)<<7);
        section_data[off+0]=(uint8_t)(instr&0xFF);
        section_data[off+1]=(uint8_t)((instr>>8)&0xFF);
        section_data[off+2]=(uint8_t)((instr>>16)&0xFF);
        section_data[off+3]=(uint8_t)((instr>>24)&0xFF);
        break;
    }
    default:
        errors_add(errors, 0, "Unknown relocation type: %d", (int)rel->type);
        return false;
    }
    return true;
}

/* ------------------------------------------------------------------ */
/*  Main linker function                                              */
/* ------------------------------------------------------------------ */
bool linker_link(int obj_count, const char **obj_paths,
                 const LinkerConfig *config,
                 MemoryImage *output,
                 uint32_t *entry_point,
                 uint32_t *min_used,
                 uint32_t *max_used,
                 ErrorList *errors)
{
    ObjectFile *objs = calloc((size_t)obj_count, sizeof(ObjectFile));
    if (!objs) { errors_add(errors,0,"Out of memory"); return false; }

    /* Step 1: Read all object files */
    for (int i=0; i<obj_count; i++) {
        if (!read_object_file(obj_paths[i], &objs[i])) {
            errors_add(errors, 0, "Failed to read: %s", obj_paths[i]);
            for (int j=0; j<i; j++) object_file_free(&objs[j]);
            free(objs); return false;
        }
    }

    /* Step 2: Calculate section layout */
    uint32_t *text_offsets = calloc((size_t)obj_count, sizeof(uint32_t));
    uint32_t *data_offsets = calloc((size_t)obj_count, sizeof(uint32_t));

    uint32_t text_cursor = config->text_start;
    for (int i=0; i<obj_count; i++) {
        /* Align to 4 bytes */
        text_cursor = (text_cursor + 3u) & ~3u;
        text_offsets[i] = text_cursor;
        text_cursor += objs[i].text.size;
    }

    uint32_t data_cursor = config->data_start_auto ? ((text_cursor + 3u) & ~3u) : config->data_start;
    for (int i=0; i<obj_count; i++) {
        data_cursor = (data_cursor + 3u) & ~3u;
        data_offsets[i] = data_cursor;
        data_cursor += objs[i].data.size;
    }

    /* Check memory bounds */
    if (data_cursor > config->memory_size) {
        errors_add(errors, 0, "Program exceeds memory: needs 0x%X, have 0x%X", data_cursor, config->memory_size);
    }

    printf("Linker: Section layout\n");
    for (int i=0; i<obj_count; i++) {
        printf("  [%d] %s: .text @ 0x%08X (%u bytes), .data @ 0x%08X (%u bytes)\n",
               i, objs[i].source_name, text_offsets[i], objs[i].text.size,
               data_offsets[i], objs[i].data.size);
    }

    /* Step 3: Build global symbol table */
    GlobalSymbolTable gst;
    gst_init(&gst);

    for (int i=0; i<obj_count; i++) {
        SymbolTable *st = &objs[i].symbols;
        for (size_t j=0; j<st->capacity; j++) {
            Symbol *sym = &st->items[j];
            if (!sym->occupied) continue;
            if (sym->binding == BIND_GLOBAL && sym->defined) {
                /* Calculate final address */
                uint32_t base = (sym->section == SEC_TEXT) ? text_offsets[i] : data_offsets[i];
                uint32_t orig_base = (sym->section == SEC_TEXT) ? objs[i].text.base_address : objs[i].data.base_address;
                uint32_t final_addr = base + (sym->address - orig_base);

                GlobalSymbol *gs = gst_find(&gst, sym->name);
                if (gs && gs->defined) {
                    errors_add(errors, 0, "Multiple definition of '%s' in %s and %s",
                               sym->name, objs[gs->defining_obj].source_name, objs[i].source_name);
                    goto cleanup;
                }
                if (!gs) gs = gst_add(&gst, sym->name);
                gs->final_address = final_addr;
                gs->defining_obj = i;
                gs->defined = true;
                gs->section = sym->section;
            }
            else if (sym->binding == BIND_EXTERN) {
                GlobalSymbol *gs = gst_find(&gst, sym->name);
                if (!gs) gst_add(&gst, sym->name); /* placeholder */
            }
        }
    }

    /* Also add local symbols for internal relocation resolution */
    /* Check all extern references are resolved */
    for (int i=0; i<obj_count; i++) {
        SymbolTable *st = &objs[i].symbols;
        for (size_t j=0; j<st->capacity; j++) {
            Symbol *sym = &st->items[j];
            if (!sym->occupied) continue;
            if (sym->binding == BIND_EXTERN && !sym->defined) {
                GlobalSymbol *gs = gst_find(&gst, sym->name);
                if (!gs || !gs->defined) {
                    errors_add(errors, 0, "Undefined symbol '%s' referenced in %s",
                               sym->name, objs[i].source_name);
                    goto cleanup;
                }
            }
        }
    }

    printf("Linker: Global symbols resolved\n");
    for (size_t i=0; i<gst.count; i++) {
        if (gst.items[i].defined)
            printf("  %-20s = 0x%08X\n", gst.items[i].name, gst.items[i].final_address);
    }

    /* Step 4: Initialize output memory image and copy sections */
    memory_image_init(output, config->memory_size);
    *min_used = UINT32_MAX;
    *max_used = 0;
    *entry_point = config->text_start;

    for (int i=0; i<obj_count; i++) {
        /* Copy text section */
        for (uint32_t b=0; b<objs[i].text.size; b++) {
            uint32_t addr = text_offsets[i] + b;
            memory_write_byte(output, addr, objs[i].text.data[b]);
            if (addr < *min_used) *min_used = addr;
            if (addr > *max_used) *max_used = addr;
        }
        /* Copy data section */
        for (uint32_t b=0; b<objs[i].data.size; b++) {
            uint32_t addr = data_offsets[i] + b;
            memory_write_byte(output, addr, objs[i].data.data[b]);
            if (addr < *min_used) *min_used = addr;
            if (addr > *max_used) *max_used = addr;
        }
    }

    /* Step 5: Apply relocations */
    for (int i=0; i<obj_count; i++) {
        /* Text relocations */
        for (size_t r=0; r<objs[i].text_relocs.count; r++) {
            RelocationEntry *rel = &objs[i].text_relocs.items[r];
            /* Resolve symbol */
            uint32_t sym_addr = 0;
            /* Try global first */
            GlobalSymbol *gs = gst_find(&gst, rel->symbol_name);
            if (gs && gs->defined) {
                sym_addr = gs->final_address;
            } else {
                /* Try local symbol in same object */
                Symbol *ls = symbol_table_find(&objs[i].symbols, rel->symbol_name);
                if (ls && ls->defined) {
                    uint32_t base = (ls->section == SEC_TEXT) ? text_offsets[i] : data_offsets[i];
                    uint32_t orig = (ls->section == SEC_TEXT) ? objs[i].text.base_address : objs[i].data.base_address;
                    sym_addr = base + (ls->address - orig);
                } else {
                    errors_add(errors, 0, "Cannot resolve '%s' in %s", rel->symbol_name, objs[i].source_name);
                    goto cleanup;
                }
            }

            uint32_t reloc_abs = text_offsets[i] + rel->offset;
            /* Apply to output memory directly */
            if (!apply_relocation(output->data + text_offsets[i], objs[i].text.size,
                                  rel, sym_addr, reloc_abs, errors))
                goto cleanup;
        }

        /* Data relocations */
        for (size_t r=0; r<objs[i].data_relocs.count; r++) {
            RelocationEntry *rel = &objs[i].data_relocs.items[r];
            uint32_t sym_addr = 0;
            GlobalSymbol *gs = gst_find(&gst, rel->symbol_name);
            if (gs && gs->defined) {
                sym_addr = gs->final_address;
            } else {
                Symbol *ls = symbol_table_find(&objs[i].symbols, rel->symbol_name);
                if (ls && ls->defined) {
                    uint32_t base = (ls->section == SEC_TEXT) ? text_offsets[i] : data_offsets[i];
                    uint32_t orig = (ls->section == SEC_TEXT) ? objs[i].text.base_address : objs[i].data.base_address;
                    sym_addr = base + (ls->address - orig);
                } else {
                    errors_add(errors, 0, "Cannot resolve '%s' in %s", rel->symbol_name, objs[i].source_name);
                    goto cleanup;
                }
            }
            uint32_t reloc_abs = data_offsets[i] + rel->offset;
            if (!apply_relocation(output->data + data_offsets[i], objs[i].data.size,
                                  rel, sym_addr, reloc_abs, errors))
                goto cleanup;
        }
    }

    /* Find _start symbol as entry point */
    GlobalSymbol *start = gst_find(&gst, "_start");
    if (start && start->defined) {
        *entry_point = start->final_address;
    }

    if (*min_used == UINT32_MAX) { *min_used = 0; *max_used = 0; }

    printf("Linker: Success. Entry=0x%08X Range=0x%08X-0x%08X\n", *entry_point, *min_used, *max_used);

    gst_free(&gst);
    free(text_offsets); free(data_offsets);
    for (int i=0; i<obj_count; i++) object_file_free(&objs[i]);
    free(objs);
    return errors->count == 0;

cleanup:
    gst_free(&gst);
    free(text_offsets); free(data_offsets);
    for (int i=0; i<obj_count; i++) object_file_free(&objs[i]);
    free(objs);
    return false;
}
