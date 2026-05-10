#include "linker.h"


/* JSON string degerini bul: "key": VALUE */
char* json_find_key(char* buf, const char* key) {
    char search[256];
    snprintf(search, sizeof(search), "\"%s\"", key);
    char* p = strstr(buf, search);
    if (!p) return NULL;
    p += strlen(search);
    while (*p == ' ' || *p == ':' || *p == '\t') p++;
    return p;
}

/* JSON string degerini oku: "value" */
int json_read_string(char* p, char* out, int maxlen) {
    if (*p != '"') return 0;
    p++;
    int i = 0;
    while (*p && *p != '"' && i < maxlen-1) out[i++] = *p++;
    out[i] = '\0';
    return 1;
}

/* uint32 array oku: [n1,n2,...] */
int json_read_uint32_array(char* p, uint32_t* arr, int maxn) {
    if (*p != '[') return 0;
    p++;
    int cnt = 0;
    while (*p && *p != ']' && cnt < maxn) {
        while (*p == ' ' || *p == '\n' || *p == '\r') p++;
        if (*p == ']') break;
        char* end;
        unsigned long v = strtoul(p, &end, 10);
        if (end == p) break;
        arr[cnt++] = (uint32_t)v;
        p = end;
        while (*p == ' ' || *p == ',') p++;
    }
    return cnt;
}

/* int array oku: [n1,n2,...] -> byte array */
int json_read_byte_array(char* p, uint8_t* arr, int maxn) {
    if (*p != '[') return 0;
    p++;
    int cnt = 0;
    while (*p && *p != ']' && cnt < maxn) {
        while (*p == ' ' || *p == '\n' || *p == '\r') p++;
        if (*p == ']') break;
        char* end;
        long v = strtol(p, &end, 10);
        if (end == p) break;
        arr[cnt++] = (uint8_t)v;
        p = end;
        while (*p == ' ' || *p == ',') p++;
    }
    return cnt;
}

/* reloc tipi strden int'e */
int parse_reloc_type(const char* s) {
    if (strcmp(s,"BRANCH")==0)     return 0;
    if (strcmp(s,"JAL")==0)        return 1;
    if (strcmp(s,"CALL")==0)       return 2;
    if (strcmp(s,"LA")==0)         return 3;
    if (strcmp(s,"LI")==0)         return 4;
    if (strcmp(s,"HI20")==0)       return 5;
    if (strcmp(s,"LO12")==0)       return 6;
    if (strcmp(s,"PCREL_HI20")==0) return 7;
    if (strcmp(s,"ABS32")==0)      return 8;
    return -1;
}

const char* reloc_type_str[] = {
    "BRANCH","JAL","CALL","LA","LI","HI20","LO12","PCREL_HI20","ABS32"
};


int linker_load_object(Linker* l, const char* path) {
    if (l->obj_count >= MAX_OBJECTS) {
        fprintf(stderr, "[ERR] Maksimum obje sayisi asild\n"); return -1;
    }

    FILE* f = fopen(path, "r");
    if (!f) { fprintf(stderr, "[ERR] Dosya acilamadi: %s\n", path); return -1; }

    /* Tum dosyayi oku */
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return -1; }
    long fsize = ftell(f);
    if (fsize < 0) { fclose(f); return -1; }
    rewind(f);
    char* buf = (char*)malloc(fsize + 1);
    if (!buf) { fclose(f); return -1; }
    if (fread(buf, 1, (size_t)fsize, f) != (size_t)fsize) {
        fprintf(stderr, "[ERR] Dosya okunamadi: %s\n", path);
        free(buf);
        fclose(f);
        return -1;
    }
    buf[fsize] = '\0';
    fclose(f);

    LoadedObject* obj = &l->objects[l->obj_count];
    memset(obj, 0, sizeof(LoadedObject));
    linker_copy_cstr(obj->filename, sizeof(obj->filename), path);

    uint32_t tmp_text[65536];
    static uint8_t  tmp_data[65536];

    char* p = json_find_key(buf, "text");
    int tc = p ? json_read_uint32_array(p, tmp_text, 65536) : 0;
    p = json_find_key(buf, "data");
    int dc = p ? json_read_byte_array(p, tmp_data, 65536) : 0;

    if (tc > MAX_TOTAL_TEXT - l->final_text_count) {
        fprintf(stderr, "[ERR] Text alani doldu: %s\n", path);
        free(buf);
        return -1;
    }
    if (dc > MAX_TOTAL_DATA - l->final_data_size) {
        fprintf(stderr, "[ERR] Data alani doldu: %s\n", path);
        free(buf);
        return -1;
    }

    obj->text_count = tc;
    obj->data_size  = dc;

    /* text'i final_text'e kopyala (offset kaydet) */
    uint32_t t_off = (uint32_t)l->final_text_count;
    memcpy(l->final_text + l->final_text_count, tmp_text, tc * sizeof(uint32_t));
    l->final_text_count += tc;

    /* data'yi final_data'ya kopyala */
    uint32_t d_off = (uint32_t)l->final_data_size;
    memcpy(l->final_data + l->final_data_size, tmp_data, dc);
    l->final_data_size += dc;
    /* 4-byte align */
    while (l->final_data_size % 4) {
        if (l->final_data_size >= MAX_TOTAL_DATA) {
            fprintf(stderr, "[ERR] Data hizalama alani doldu: %s\n", path);
            free(buf);
            return -1;
        }
        l->final_data[l->final_data_size++] = 0;
    }

    /* base adresler (Pass 1'de doldurulacak) - simdilik ofset sakla */
    obj->text_base = t_off;  /* gecici: word ofseti */
    obj->data_base = d_off;  /* gecici: byte ofseti */

    /* Sembolleri parse et */
    /* symbols: {"name":{"section":"...","offset":N,"global":...}, ...} */
    char* sym_sec = json_find_key(buf, "symbols");
    if (sym_sec && *sym_sec == '{') {
        char* sp = sym_sec + 1;
        while (*sp && *sp != '}') {
            while (*sp == ' '||*sp=='\n'||*sp=='\r'||*sp==',') sp++;
            if (*sp != '"') break;
            /* isim */
            char sname[MAX_NAME_LEN] = {0};
            json_read_string(sp, sname, MAX_NAME_LEN);
            sp = strchr(sp+1, '"');
            if (!sp) break;
            sp++;
            /* : { */
            sp = strchr(sp, '{');
            if (!sp) break;
            char* end_obj = strchr(sp, '}');
            if (!end_obj) break;
            char obj_buf[512]; int olen = (int)(end_obj - sp + 1);
            if (olen >= 512) olen = 511;
            memcpy(obj_buf, sp, (size_t)olen); obj_buf[olen] = '\0';

            char sec[16] = {0}; uint32_t soff = 0; int is_global = 0;
            char* pp = json_find_key(obj_buf, "section");
            if (pp) json_read_string(pp, sec, 16);
            pp = json_find_key(obj_buf, "offset");
            if (pp) soff = (uint32_t)strtol(pp, NULL, 10);
            pp = json_find_key(obj_buf, "global");
            if (pp && strncmp(pp,"true",4)==0) is_global = 1;

            if (is_global && obj->gsym_count < 256) {
                linker_copy_cstr(obj->gsyms[obj->gsym_count].name,
                                 sizeof(obj->gsyms[obj->gsym_count].name), sname);
                linker_copy_cstr(obj->gsyms[obj->gsym_count].section,
                                 sizeof(obj->gsyms[obj->gsym_count].section), sec);
                obj->gsyms[obj->gsym_count].offset = soff;
                obj->gsym_count++;
            } else if (obj->lsym_count < 512) {
                linker_copy_cstr(obj->lsyms[obj->lsym_count].name,
                                 sizeof(obj->lsyms[obj->lsym_count].name), sname);
                linker_copy_cstr(obj->lsyms[obj->lsym_count].section,
                                 sizeof(obj->lsyms[obj->lsym_count].section), sec);
                obj->lsyms[obj->lsym_count].offset = soff;
                obj->lsym_count++;
            }
            sp = end_obj + 1;
        }
    }

    /* Relocations parse et */
    char* rel_sec = json_find_key(buf, "relocations");
    if (rel_sec && *rel_sec == '[') {
        char* rp = rel_sec + 1;
        while (*rp && *rp != ']' && obj->reloc_count < MAX_RELOCS) {
            while (*rp==' '||*rp=='\n'||*rp=='\r'||*rp==',') rp++;
            if (*rp != '{') break;
            char* re = strchr(rp, '}');
            if (!re) break;
            char rbuf[512]; int rlen = (int)(re - rp + 1);
            if (rlen >= 512) rlen = 511;
            memcpy(rbuf, rp, (size_t)rlen); rbuf[rlen] = '\0';

            LReloc* r = &obj->relocs[obj->reloc_count++];
            memset(r, 0, sizeof(LReloc));

            char* pp = json_find_key(rbuf, "offset");
            if (pp) r->offset = (uint32_t)strtol(pp, NULL, 10);
            pp = json_find_key(rbuf, "section");
            if (pp) json_read_string(pp, r->section, (int)sizeof(r->section));
            pp = json_find_key(rbuf, "type");
            if (pp) { char ts[32]={0}; json_read_string(pp,ts,32); r->type=parse_reloc_type(ts); }
            pp = json_find_key(rbuf, "symbol");
            if (pp) json_read_string(pp, r->symbol, MAX_NAME_LEN);
            pp = json_find_key(rbuf, "addend");
            if (pp) r->addend = (int32_t)strtol(pp, NULL, 10);

            rp = re + 1;
        }
    }

    printf("[LINK] Yuklendi: %s | text:%d komut, data:%d byte, sym:%d, reloc:%d\n",
           path, tc, dc, obj->gsym_count + obj->lsym_count, obj->reloc_count);

    free(buf);
    l->obj_count++;
    return 0;
}
