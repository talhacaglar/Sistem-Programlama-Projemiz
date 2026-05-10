#include "linker.h"

uint32_t patch_B(uint32_t instr, int32_t imm) {
    imm &= 0x1FFF;
    int b12=(imm>>12)&1, b11=(imm>>11)&1, b10_5=(imm>>5)&0x3F, b4_1=(imm>>1)&0xF;
    instr &= 0x01FFF07F;
    return instr|(b12<<31)|(b10_5<<25)|(b4_1<<8)|(b11<<7);
}
uint32_t patch_J(uint32_t instr, int32_t imm) {
    imm &= 0x1FFFFF;
    int b20=(imm>>20)&1, b19_12=(imm>>12)&0xFF, b11=(imm>>11)&1, b10_1=(imm>>1)&0x3FF;
    instr &= 0x00000FFF;
    return instr|(b20<<31)|(b10_1<<21)|(b11<<20)|(b19_12<<12);
}
uint32_t patch_I_imm(uint32_t instr, int32_t imm) {
    imm &= 0xFFF;
    return (instr & 0x000FFFFF) | ((uint32_t)imm << 20);
}
uint32_t patch_U(uint32_t instr, int32_t imm) {
    int32_t upper = (imm >> 12) & 0xFFFFF;
    return (instr & 0xFFF) | ((uint32_t)(upper & 0xFFFFF) << 12);
}


void apply_reloc(uint32_t* text_words, int text_count,
                        LReloc* r, uint32_t sym_addr, uint32_t instr_addr) {
    int widx = (int)(r->offset / 4);
    if (widx >= text_count) {
        fprintf(stderr, "  [ERR] Reloc ofset sinir disi: %u\n", r->offset);
        return;
    }
    uint32_t instr = text_words[widx];
    int rtype = r->type;

    if (rtype == 0) { /* BRANCH */
        int32_t off = (int32_t)(sym_addr - instr_addr);
        text_words[widx] = patch_B(instr, off);
    }
    else if (rtype == 1) { /* JAL */
        int32_t off = (int32_t)(sym_addr - instr_addr);
        text_words[widx] = patch_J(instr, off);
    }
    else if (rtype == 2) { /* CALL: auipc ra + jalr ra */
        int32_t off = (int32_t)(sym_addr - instr_addr);
        int32_t hi = (off + 0x800) >> 12;
        int32_t lo = off - (hi << 12);
        text_words[widx]   = patch_U(text_words[widx], hi << 12);
        if (widx+1 < text_count)
            text_words[widx+1] = patch_I_imm(text_words[widx+1], lo);
    }
    else if (rtype == 3) { /* LA: auipc + addi */
        int32_t off = (int32_t)(sym_addr - instr_addr);
        int32_t hi = (off + 0x800) >> 12;
        int32_t lo = off - (hi << 12);
        text_words[widx]   = patch_U(text_words[widx], hi << 12);
        if (widx+1 < text_count)
            text_words[widx+1] = patch_I_imm(text_words[widx+1], lo);
    }
    else if (rtype == 4) { /* LI: lui + addi */
        int32_t hi = ((int32_t)sym_addr + 0x800) >> 12;
        int32_t lo = (int32_t)sym_addr - (hi << 12);
        text_words[widx]   = patch_U(text_words[widx], hi << 12);
        if (widx+1 < text_count)
            text_words[widx+1] = patch_I_imm(text_words[widx+1], lo);
    }
    else if (rtype == 5) { /* HI20 */
        int32_t hi = ((int32_t)sym_addr + 0x800) >> 12;
        text_words[widx] = patch_U(instr, hi << 12);
    }
    else if (rtype == 6) { /* LO12 */
        int32_t lo = (int32_t)sym_addr & 0xFFF;
        text_words[widx] = patch_I_imm(instr, lo);
    }
    else if (rtype == 7) { /* PCREL_HI20 */
        int32_t off = (int32_t)(sym_addr - instr_addr);
        int32_t hi = (off + 0x800) >> 12;
        text_words[widx] = patch_U(instr, hi << 12);
    }
    else if (rtype == 8) { /* ABS32 - data icinde */
        /* data patch ayri yapilir */
    }
}

