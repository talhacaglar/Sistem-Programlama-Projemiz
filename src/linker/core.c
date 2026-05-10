#include "linker.h"

void linker_init(Linker* l, uint32_t text_base, uint32_t data_base, uint32_t stack_top) {
    memset(l, 0, sizeof(Linker));
    l->text_base = text_base;
    l->data_base = data_base;
    l->stack_top = stack_top;
}


uint32_t sym_resolve(SymbolTable* t, const char* name) {
    for (int i = 0; i < t->count; i++)
        if (strcmp(t->syms[i].name, name) == 0)
            return t->syms[i].address;
    return 0xFFFFFFFF;
}

void sym_add(SymbolTable* t, const char* name, uint32_t addr, const char* file) {
    /* cakisma kontrolu */
    for (int i = 0; i < t->count; i++) {
        if (strcmp(t->syms[i].name, name) == 0) {
            fprintf(stderr, "  [WARN] Sembol cakismasi: '%s'\n", name);
            return;
        }
    }
    if (t->count >= MAX_GLOBAL_SYMS) { fprintf(stderr,"[ERR] Sembol tablosu doldu\n"); return; }
    GlobalSymbol* s = &t->syms[t->count++];
    linker_copy_cstr(s->name, sizeof(s->name), name);
    s->address = addr;
    linker_copy_cstr(s->src_file, sizeof(s->src_file), file);
    printf("  [SYM] %-30s = 0x%08x  [%s]\n", name, addr, file);
}

static uint32_t resolve_object_symbol(Linker* l, LoadedObject* obj, const char* name) {
    uint32_t sym_addr = sym_resolve(&l->sym_table, name);
    if (sym_addr != 0xFFFFFFFF) return sym_addr;

    for (int k = 0; k < obj->lsym_count; k++) {
        if (strcmp(obj->lsyms[k].name, name) == 0) {
            if (strcmp(obj->lsyms[k].section, "text") == 0)
                return obj->text_base + obj->lsyms[k].offset;
            return obj->data_base + obj->lsyms[k].offset;
        }
    }

    for (int k = 0; k < obj->gsym_count; k++) {
        if (strcmp(obj->gsyms[k].name, name) == 0) {
            if (strcmp(obj->gsyms[k].section, "text") == 0)
                return obj->text_base + obj->gsyms[k].offset;
            return obj->data_base + obj->gsyms[k].offset;
        }
    }

    return 0xFFFFFFFF;
}


void linker_pass1(Linker* l) {
    printf("\n[LINK] Pass 1: Sembol tablosu...\n");

    /* Gecici ofsetleri gercek adreslere cevir */
    for (int i = 0; i < l->obj_count; i++) {
        LoadedObject* obj = &l->objects[i];

        /* text_base ve data_base'i duzelt */
        /* obj->text_base simdiye kadar word ofseti olarak saklandı */
        uint32_t real_text_base = l->text_base + obj->text_base * 4;
        uint32_t real_data_base = l->data_base + obj->data_base;
        obj->text_base = real_text_base;
        obj->data_base = real_data_base;

        /* Global sembolleri ekle */
        for (int j = 0; j < obj->gsym_count; j++) {
            uint32_t abs_addr;
            if (strcmp(obj->gsyms[j].section, "text") == 0)
                abs_addr = real_text_base + obj->gsyms[j].offset;
            else
                abs_addr = real_data_base + obj->gsyms[j].offset;
            sym_add(&l->sym_table, obj->gsyms[j].name, abs_addr, obj->filename);
        }

        /* Local semboller - sadece bu obje icinde kullanilir */
        /* (linker bunlari global tabloya eklemez, pass2'de lokal arama yapilir) */
    }

    /* _start yoksa ilk text base'i gir */
    if (sym_resolve(&l->sym_table, "_start") == 0xFFFFFFFF && l->obj_count > 0)
        sym_add(&l->sym_table, "_start", l->text_base, "varsayilan");
}


int linker_pass2(Linker* l) {
    printf("\n[LINK] Pass 2: Relocation uygulaniyor...\n");
    int errors = 0;

    for (int i = 0; i < l->obj_count; i++) {
        LoadedObject* obj = &l->objects[i];
        /* Bu objenin text'i final_text'te hangi indeksten basliyor? */
        int text_word_base = (int)((obj->text_base - l->text_base) / 4);

        for (int j = 0; j < obj->reloc_count; j++) {
            LReloc* r = &obj->relocs[j];
            if (strcmp(r->section, "data") == 0) continue; /* data ayri */

            if (r->type < 0 || r->type > 8) {
                fprintf(stderr, "  [ERR] Gecersiz relocation tipi: %d (%s)\n", r->type, obj->filename);
                errors++;
                continue;
            }

            uint32_t sym_addr = resolve_object_symbol(l, obj, r->symbol);

            if (sym_addr == 0xFFFFFFFF) {
                fprintf(stderr, "  [ERR] Cozulemeyen sembol: '%s' (%s)\n", r->symbol, obj->filename);
                errors++;
                continue;
            }

            uint32_t instr_addr = obj->text_base + r->offset;
            int global_widx = text_word_base + (int)(r->offset / 4);
            if (global_widx < 0 || global_widx >= l->final_text_count) {
                fprintf(stderr, "  [ERR] Reloc ofset sinir disi: %u (%s)\n",
                        r->offset, obj->filename);
                errors++;
                continue;
            }

            printf("  [REL] %-12s %-25s -> 0x%08x\n",
                   reloc_type_str[r->type], r->symbol, sym_addr);

            /* text reloc: final_text array'ini patch'le */
            /* gecici text kopyasi uzerinde calis */
            LReloc tmp = *r;
            tmp.offset = (uint32_t)(global_widx * 4);
            apply_reloc(l->final_text, l->final_text_count, &tmp, sym_addr, instr_addr);
        }

        /* Data ABS32 reloc */
        int data_byte_base = (int)(obj->data_base - l->data_base);
        for (int j = 0; j < obj->reloc_count; j++) {
            LReloc* r = &obj->relocs[j];
            if (strcmp(r->section, "data") != 0 || r->type != 8) continue;

            uint32_t sym_addr = resolve_object_symbol(l, obj, r->symbol);
            if (sym_addr == 0xFFFFFFFF) {
                fprintf(stderr, "  [ERR] Data ABS32 cozulemeyen sembol: '%s'\n", r->symbol);
                errors++;
                continue;
            }
            int off = data_byte_base + (int)r->offset;
            if (off < 0 || off + 3 >= l->final_data_size) {
                fprintf(stderr, "  [ERR] Data ABS32 ofset sinir disi: %s + %u\n",
                        obj->filename, r->offset);
                errors++;
                continue;
            }
            l->final_data[off+0] = sym_addr & 0xFF;
            l->final_data[off+1] = (sym_addr>>8) & 0xFF;
            l->final_data[off+2] = (sym_addr>>16) & 0xFF;
            l->final_data[off+3] = (sym_addr>>24) & 0xFF;
        }
    }

    return errors == 0 ? 0 : -1;
}
