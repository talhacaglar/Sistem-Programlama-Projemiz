#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "assembler.h"
#include "opcodes.h"
#include "utils.h"
#include "objfile.h"

static bool is_blank_operands(const char *s)
{
    while (*s) {
        if (!isspace((unsigned char)*s)) return false;
        ++s;
    }
    return true;
}

static bool parse_mem_operand(const char *text, int64_t *imm, int *rs1, const SymbolTable *symbols)
{
    char buf[96];
    strncpy(buf, text, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    trim(buf);
    char *lp = strchr(buf, '(');
    char *rp = strchr(buf, ')');
    if (!lp || !rp || rp < lp) return false;
    *lp = '\0'; *rp = '\0';
    char imm_text[64], reg_text[32];
    strncpy(imm_text, buf, sizeof(imm_text)-1); imm_text[sizeof(imm_text)-1]='\0';
    strncpy(reg_text, lp+1, sizeof(reg_text)-1); reg_text[sizeof(reg_text)-1]='\0';
    trim(imm_text); trim(reg_text);
    if (imm_text[0] == '\0') { *imm = 0; }
    else {
        bool is_sym = false;
        if (!eval_expr(imm_text, symbols, imm, &is_sym)) return false;
    }
    *rs1 = parse_register(reg_text);
    return *rs1 >= 0;
}

static bool validate_range(ErrorList *e, int ln, int64_t v, int64_t lo, int64_t hi, const char *w)
{
    if (v < lo || v > hi) { errors_add(e, ln, "%s out of range: %lld", w, (long long)v); return false; }
    return true;
}

static uint32_t encode_r(uint8_t op,uint8_t f3,uint8_t f7,int rd,int r1,int r2){
    return ((uint32_t)f7<<25)|((uint32_t)r2<<20)|((uint32_t)r1<<15)|((uint32_t)f3<<12)|((uint32_t)rd<<7)|op;}
static uint32_t encode_i(uint8_t op,uint8_t f3,int rd,int r1,int32_t imm){
    return (((uint32_t)imm&0xFFFu)<<20)|((uint32_t)r1<<15)|((uint32_t)f3<<12)|((uint32_t)rd<<7)|op;}
static uint32_t encode_shift_i(uint8_t op,uint8_t f3,uint8_t f7,int rd,int r1,uint8_t sh){
    uint32_t imm=((uint32_t)f7<<5)|(uint32_t)(sh&0x1Fu);
    return (imm<<20)|((uint32_t)r1<<15)|((uint32_t)f3<<12)|((uint32_t)rd<<7)|op;}
static uint32_t encode_s(uint8_t op,uint8_t f3,int r1,int r2,int32_t imm){
    uint32_t i=(uint32_t)imm&0xFFFu;
    return (((i>>5)&0x7Fu)<<25)|((uint32_t)r2<<20)|((uint32_t)r1<<15)|((uint32_t)f3<<12)|((i&0x1Fu)<<7)|op;}
static uint32_t encode_b(uint8_t op,uint8_t f3,int r1,int r2,int32_t off){
    uint32_t i=(uint32_t)off&0x1FFFu;
    return (((i>>12)&1u)<<31)|(((i>>5)&0x3Fu)<<25)|((uint32_t)r2<<20)|((uint32_t)r1<<15)|
           ((uint32_t)f3<<12)|(((i>>1)&0xFu)<<8)|(((i>>11)&1u)<<7)|op;}
static uint32_t encode_u(uint8_t op,int rd,int32_t imm20){
    return ((uint32_t)imm20&0xFFFFF000u)|((uint32_t)rd<<7)|op;}
static uint32_t encode_j(uint8_t op,int rd,int32_t off){
    uint32_t i=(uint32_t)off&0x1FFFFFu;
    return (((i>>20)&1u)<<31)|(((i>>1)&0x3FFu)<<21)|(((i>>11)&1u)<<20)|(((i>>12)&0xFFu)<<12)|((uint32_t)rd<<7)|op;}

/* ---- Directive emission (pass2) ---- */
static bool emit_directive_pass2(ParsedLine *line, const SymbolTable *symbols, ErrorList *errors)
{
    line->emits_bytes = false;
    line->emitted_size = 0;
    if (line->mnemonic[0] == '\0') return true;
    if (strcmp(line->mnemonic,".text")==0||strcmp(line->mnemonic,".data")==0||
        strcmp(line->mnemonic,".org")==0||strcmp(line->mnemonic,".end")==0||
        strcmp(line->mnemonic,".global")==0||strcmp(line->mnemonic,".extern")==0)
        return true;

    if (strcmp(line->mnemonic, ".word") == 0) {
        char ops[64][64]; int n = split_operands(line->operands, ops, 64);
        for (int i = 0; i < n; ++i) {
            int64_t value = 0; bool symbolic = false;
            if (!eval_expr(ops[i], symbols, &value, &symbolic)) {
                errors_add(errors, line->line_no, "Invalid .word operand: %s", ops[i]); return false;
            }
            if (line->emitted_size + 4 > sizeof(line->bytes)) {
                errors_add(errors, line->line_no, ".word overflow"); return false;
            }
            uint32_t v = (uint32_t)value;
            line->bytes[line->emitted_size+0]=(uint8_t)(v&0xFF);
            line->bytes[line->emitted_size+1]=(uint8_t)((v>>8)&0xFF);
            line->bytes[line->emitted_size+2]=(uint8_t)((v>>16)&0xFF);
            line->bytes[line->emitted_size+3]=(uint8_t)((v>>24)&0xFF);
            line->emitted_size += 4;
        }
        line->emits_bytes = true; return true;
    }
    if (strcmp(line->mnemonic, ".byte") == 0) {
        char ops[128][64]; int n = split_operands(line->operands, ops, 128);
        for (int i = 0; i < n; ++i) {
            int64_t value = 0; bool symbolic = false;
            if (!eval_expr(ops[i], symbols, &value, &symbolic)) {
                errors_add(errors, line->line_no, "Invalid .byte operand: %s", ops[i]); return false;
            }
            if (!validate_range(errors, line->line_no, value, -128, 255, ".byte value")) return false;
            if (line->emitted_size >= sizeof(line->bytes)) {
                errors_add(errors, line->line_no, ".byte overflow"); return false;
            }
            line->bytes[line->emitted_size++] = (uint8_t)value;
        }
        line->emits_bytes = true; return true;
    }
    errors_add(errors, line->line_no, "Unsupported directive: %s", line->mnemonic);
    return false;
}

/* ---- Pass 1 ---- */
bool run_pass1(LineVector *lines, SymbolTable *symbols, ErrorList *errors, uint32_t *program_end)
{
    uint32_t locctr = 0;
    Section current = SEC_TEXT;

    for (size_t i = 0; i < lines->count; ++i) {
        ParsedLine *line = &lines->items[i];
        line->section = current;
        line->address = locctr;
        line->emits_bytes = false;
        line->emitted_size = 0;
        if (line->is_empty) continue;

        if (line->label[0] != '\0') {
            if (!symbol_table_add(symbols, line->label, locctr, current, false)) {
                errors_add(errors, line->line_no, "Duplicate label: %s", line->label);
                return false;
            }
        }
        if (line->mnemonic[0] == '\0') continue;

        if (line->is_directive) {
            if (strcmp(line->mnemonic, ".text") == 0) {
                current = SEC_TEXT; line->section = current;
            } else if (strcmp(line->mnemonic, ".data") == 0) {
                current = SEC_DATA; line->section = current;
            } else if (strcmp(line->mnemonic, ".org") == 0) {
                int64_t value = 0; bool symbolic = false;
                if (!eval_expr(line->operands, symbols, &value, &symbolic)) {
                    errors_add(errors, line->line_no, "Invalid .org: %s", line->operands); return false;
                }
                if (!validate_range(errors, line->line_no, value, 0, 0xFFFFFFFFll, ".org address")) return false;
                locctr = (uint32_t)value; line->address = locctr;
            } else if (strcmp(line->mnemonic, ".word") == 0) {
                char ops[64][64]; int n = split_operands(line->operands, ops, 64);
                locctr += (uint32_t)(n * 4);
            } else if (strcmp(line->mnemonic, ".byte") == 0) {
                char ops[128][64]; int n = split_operands(line->operands, ops, 128);
                locctr += (uint32_t)n;
            } else if (strcmp(line->mnemonic, ".global") == 0) {
                /* Mark symbol as global - will be resolved after all labels are collected */
                char name[MAX_LABEL_LEN+1];
                strncpy(name, line->operands, MAX_LABEL_LEN); name[MAX_LABEL_LEN]='\0';
                trim(name);
                /* Store for later processing - symbol may not exist yet */
            } else if (strcmp(line->mnemonic, ".extern") == 0) {
                char name[MAX_LABEL_LEN+1];
                strncpy(name, line->operands, MAX_LABEL_LEN); name[MAX_LABEL_LEN]='\0';
                trim(name);
                /* Add as undefined extern symbol */
                Symbol *existing = symbol_table_find(symbols, name);
                if (!existing) {
                    symbol_table_add(symbols, name, 0, SEC_TEXT, false);
                    existing = symbol_table_find(symbols, name);
                    if (existing) {
                        existing->binding = BIND_EXTERN;
                        existing->defined = false;
                    }
                }
            } else if (strcmp(line->mnemonic, ".end") == 0) {
                break;
            } else {
                errors_add(errors, line->line_no, "Unsupported directive: %s", line->mnemonic);
                return false;
            }
            continue;
        }
        const OpcodeEntry *op = find_opcode(line->mnemonic);
        if (!op) { errors_add(errors, line->line_no, "Unknown instruction: %s", line->mnemonic); return false; }
        locctr += 4u;
    }

    /* Second pass over directives: apply .global bindings */
    for (size_t i = 0; i < lines->count; ++i) {
        ParsedLine *line = &lines->items[i];
        if (line->is_directive && strcmp(line->mnemonic, ".global") == 0) {
            char name[MAX_LABEL_LEN+1];
            strncpy(name, line->operands, MAX_LABEL_LEN); name[MAX_LABEL_LEN]='\0';
            trim(name);
            Symbol *sym = symbol_table_find(symbols, name);
            if (sym) {
                sym->binding = BIND_GLOBAL;
            } else {
                errors_add(errors, line->line_no, "Undefined .global symbol: %s", name);
                return false;
            }
        }
    }

    *program_end = locctr;
    return errors->count == 0;
}

/* ---- Instruction encoding ---- */
static bool encode_instruction(ParsedLine *line, const SymbolTable *symbols, ErrorList *errors)
{
    const OpcodeEntry *op = find_opcode(line->mnemonic);
    if (!op) { errors_add(errors, line->line_no, "Unknown instruction in pass2: %s", line->mnemonic); return false; }

    char ops[4][64] = {{0}};
    int count = split_operands(line->operands, ops, 4);
    uint32_t word = 0;

    switch (op->style) {
    case OPSTYLE_RD_RS1_RS2: {
        if (count!=3){errors_add(errors,line->line_no,"%s expects rd,rs1,rs2",line->mnemonic);return false;}
        int rd=parse_register(ops[0]),r1=parse_register(ops[1]),r2=parse_register(ops[2]);
        if(rd<0||r1<0||r2<0){errors_add(errors,line->line_no,"Invalid register.");return false;}
        word=encode_r(op->opcode,op->funct3,op->funct7,rd,r1,r2); break;
    }
    case OPSTYLE_RD_RS1_IMM: {
        if(count!=3){errors_add(errors,line->line_no,"%s expects rd,rs1,imm",line->mnemonic);return false;}
        int rd=parse_register(ops[0]),r1=parse_register(ops[1]);
        int64_t imm=0; bool sym=false;
        if(rd<0||r1<0||!eval_expr(ops[2],symbols,&imm,&sym)){errors_add(errors,line->line_no,"Invalid operand in %s",line->mnemonic);return false;}
        if(strcmp(line->mnemonic,"slli")==0||strcmp(line->mnemonic,"srli")==0||strcmp(line->mnemonic,"srai")==0){
            if(!validate_range(errors,line->line_no,imm,0,31,"Shift amount"))return false;
            word=encode_shift_i(op->opcode,op->funct3,op->funct7,rd,r1,(uint8_t)imm);
        } else {
            if(!validate_range(errors,line->line_no,imm,-2048,2047,"12-bit immediate"))return false;
            word=encode_i(op->opcode,op->funct3,rd,r1,(int32_t)imm);
        } break;
    }
    case OPSTYLE_RD_IMM_RS1: {
        if(count!=2){errors_add(errors,line->line_no,"%s expects rd,imm(rs1)",line->mnemonic);return false;}
        int rd=parse_register(ops[0]),r1=-1; int64_t imm=0;
        if(rd<0||!parse_mem_operand(ops[1],&imm,&r1,symbols)){errors_add(errors,line->line_no,"Invalid mem operand");return false;}
        if(!validate_range(errors,line->line_no,imm,-2048,2047,"12-bit displacement"))return false;
        word=encode_i(op->opcode,op->funct3,rd,r1,(int32_t)imm); break;
    }
    case OPSTYLE_RS2_IMM_RS1: {
        if(count!=2){errors_add(errors,line->line_no,"%s expects rs2,imm(rs1)",line->mnemonic);return false;}
        int r2=parse_register(ops[0]),r1=-1; int64_t imm=0;
        if(r2<0||!parse_mem_operand(ops[1],&imm,&r1,symbols)){errors_add(errors,line->line_no,"Invalid mem operand");return false;}
        if(!validate_range(errors,line->line_no,imm,-2048,2047,"12-bit displacement"))return false;
        word=encode_s(op->opcode,op->funct3,r1,r2,(int32_t)imm); break;
    }
    case OPSTYLE_RS1_RS2_LABEL: {
        if(count!=3){errors_add(errors,line->line_no,"%s expects rs1,rs2,label",line->mnemonic);return false;}
        int r1=parse_register(ops[0]),r2=parse_register(ops[1]);
        int64_t target=0; bool sym=false;
        if(r1<0||r2<0||!eval_expr(ops[2],symbols,&target,&sym)){errors_add(errors,line->line_no,"Invalid branch operand.");return false;}
        int64_t off=target-(int64_t)line->address;
        if((off&1)!=0){errors_add(errors,line->line_no,"Branch target must be 2-byte aligned.");return false;}
        if(!validate_range(errors,line->line_no,off,-4096,4094,"Branch offset"))return false;
        word=encode_b(op->opcode,op->funct3,r1,r2,(int32_t)off); break;
    }
    case OPSTYLE_RD_LABEL: {
        if(count!=2){errors_add(errors,line->line_no,"%s expects rd,label",line->mnemonic);return false;}
        int rd=parse_register(ops[0]); int64_t target=0; bool sym=false;
        if(rd<0||!eval_expr(ops[1],symbols,&target,&sym)){errors_add(errors,line->line_no,"Invalid jump operand.");return false;}
        int64_t off=target-(int64_t)line->address;
        if((off&1)!=0){errors_add(errors,line->line_no,"Jump target must be 2-byte aligned.");return false;}
        if(!validate_range(errors,line->line_no,off,-(1<<20),(1<<20)-2,"JAL offset"))return false;
        word=encode_j(op->opcode,rd,(int32_t)off); break;
    }
    case OPSTYLE_RD_UIMM: {
        if(count!=2){errors_add(errors,line->line_no,"%s expects rd,imm20",line->mnemonic);return false;}
        int rd=parse_register(ops[0]); int64_t imm=0; bool sym=false;
        if(rd<0||!eval_expr(ops[1],symbols,&imm,&sym)){errors_add(errors,line->line_no,"Invalid upper immediate.");return false;}
        if(!validate_range(errors,line->line_no,imm,-524288,1048575,"U-type immediate"))return false;
        uint32_t shifted=((uint32_t)imm&0xFFFFFu)<<12;
        word=encode_u(op->opcode,rd,(int32_t)shifted); break;
    }
    case OPSTYLE_NO_OPERANDS: {
        if(!is_blank_operands(line->operands)){errors_add(errors,line->line_no,"%s does not take operands",line->mnemonic);return false;}
        if(strcmp(line->mnemonic,"ecall")==0) word=0x00000073u;
        else if(strcmp(line->mnemonic,"ebreak")==0) word=0x00100073u;
        else {errors_add(errors,line->line_no,"Unsupported: %s",line->mnemonic);return false;}
        break;
    }
    default: errors_add(errors,line->line_no,"Unsupported style in %s",line->mnemonic); return false;
    }

    line->bytes[0]=(uint8_t)(word&0xFF); line->bytes[1]=(uint8_t)((word>>8)&0xFF);
    line->bytes[2]=(uint8_t)((word>>16)&0xFF); line->bytes[3]=(uint8_t)((word>>24)&0xFF);
    line->emitted_size=4; line->emits_bytes=true;
    return true;
}

/* ---- Pass 2 ---- */
bool run_pass2(LineVector *lines, const SymbolTable *symbols, MemoryImage *image,
               ErrorList *errors, uint32_t *entry_point, uint32_t *min_used, uint32_t *max_used)
{
    *entry_point=0; *min_used=UINT32_MAX; *max_used=0;
    bool first_instr=false;

    for (size_t i = 0; i < lines->count; ++i) {
        ParsedLine *line = &lines->items[i];
        if (line->is_empty || line->mnemonic[0]=='\0') continue;
        if (line->is_directive) {
            if (strcmp(line->mnemonic,".end")==0) break;
            if (!emit_directive_pass2(line, symbols, errors)) return false;
        } else {
            if (!encode_instruction(line, symbols, errors)) return false;
            if (!first_instr) { *entry_point=line->address; first_instr=true; }
        }
        if (line->emits_bytes) {
            for (size_t j=0; j<line->emitted_size; ++j) {
                uint32_t addr=line->address+(uint32_t)j;
                if (!memory_write_byte(image, addr, line->bytes[j])) {
                    errors_add(errors, line->line_no, "Address 0x%08X exceeds memory.", addr); return false;
                }
                if (addr<*min_used) *min_used=addr;
                if (addr>*max_used) *max_used=addr;
            }
        }
    }
    if (*min_used==UINT32_MAX) { *min_used=0; *max_used=0; }
    return errors->count==0;
}

/* ---- Object file pass2: tolerates unresolved externs, generates relocations ---- */
static bool encode_instruction_obj(ParsedLine *line, const SymbolTable *symbols,
                                   RelocationList *relocs, ErrorList *errors)
{
    const OpcodeEntry *op = find_opcode(line->mnemonic);
    if (!op) { errors_add(errors, line->line_no, "Unknown instruction: %s", line->mnemonic); return false; }

    char ops_arr[4][64] = {{0}};
    int count = split_operands(line->operands, ops_arr, 4);
    uint32_t word = 0;

    switch (op->style) {
    case OPSTYLE_RD_RS1_RS2: {
        if (count!=3){errors_add(errors,line->line_no,"%s expects rd,rs1,rs2",line->mnemonic);return false;}
        int rd=parse_register(ops_arr[0]),r1=parse_register(ops_arr[1]),r2=parse_register(ops_arr[2]);
        if(rd<0||r1<0||r2<0){errors_add(errors,line->line_no,"Invalid register.");return false;}
        word=encode_r(op->opcode,op->funct3,op->funct7,rd,r1,r2); break;
    }
    case OPSTYLE_RD_RS1_IMM: {
        if(count!=3){errors_add(errors,line->line_no,"%s expects rd,rs1,imm",line->mnemonic);return false;}
        int rd=parse_register(ops_arr[0]),r1=parse_register(ops_arr[1]);
        int64_t imm=0; bool sym=false;
        if(rd<0||r1<0){errors_add(errors,line->line_no,"Invalid register.");return false;}
        if(!eval_expr(ops_arr[2],symbols,&imm,&sym)){
            /* Could be extern - create relocation with LO12_I */
            char name[MAX_LABEL_LEN+1]; strncpy(name,ops_arr[2],MAX_LABEL_LEN); name[MAX_LABEL_LEN]='\0'; trim(name);
            RelocationEntry rel; memset(&rel,0,sizeof(rel));
            rel.offset = line->address;
            rel.type = RELOC_R_RISCV_LO12_I;
            snprintf(rel.symbol_name, sizeof(rel.symbol_name), "%s", name);
            reloc_list_push(relocs, &rel);
            imm = 0; /* placeholder */
        }
        if(strcmp(line->mnemonic,"slli")==0||strcmp(line->mnemonic,"srli")==0||strcmp(line->mnemonic,"srai")==0){
            word=encode_shift_i(op->opcode,op->funct3,op->funct7,rd,r1,(uint8_t)imm);
        } else {
            word=encode_i(op->opcode,op->funct3,rd,r1,(int32_t)imm);
        } break;
    }
    case OPSTYLE_RD_IMM_RS1: {
        if(count!=2){errors_add(errors,line->line_no,"%s expects rd,imm(rs1)",line->mnemonic);return false;}
        int rd=parse_register(ops_arr[0]),r1=-1; int64_t imm=0;
        if(rd<0||!parse_mem_operand(ops_arr[1],&imm,&r1,symbols)){errors_add(errors,line->line_no,"Invalid mem operand");return false;}
        word=encode_i(op->opcode,op->funct3,rd,r1,(int32_t)imm); break;
    }
    case OPSTYLE_RS2_IMM_RS1: {
        if(count!=2){errors_add(errors,line->line_no,"%s expects rs2,imm(rs1)",line->mnemonic);return false;}
        int r2=parse_register(ops_arr[0]),r1=-1; int64_t imm=0;
        if(r2<0||!parse_mem_operand(ops_arr[1],&imm,&r1,symbols)){errors_add(errors,line->line_no,"Invalid mem operand");return false;}
        word=encode_s(op->opcode,op->funct3,r1,r2,(int32_t)imm); break;
    }
    case OPSTYLE_RS1_RS2_LABEL: {
        if(count!=3){errors_add(errors,line->line_no,"%s expects rs1,rs2,label",line->mnemonic);return false;}
        int r1=parse_register(ops_arr[0]),r2=parse_register(ops_arr[1]);
        if(r1<0||r2<0){errors_add(errors,line->line_no,"Invalid register.");return false;}
        int64_t target=0; bool sym=false;
        if(!eval_expr(ops_arr[2],symbols,&target,&sym)){
            /* Extern branch target - create relocation */
            char name[MAX_LABEL_LEN+1]; strncpy(name,ops_arr[2],MAX_LABEL_LEN); name[MAX_LABEL_LEN]='\0'; trim(name);
            RelocationEntry rel; memset(&rel,0,sizeof(rel));
            rel.offset = line->address;
            rel.type = RELOC_R_RISCV_BRANCH;
            snprintf(rel.symbol_name, sizeof(rel.symbol_name), "%s", name);
            reloc_list_push(relocs, &rel);
            word=encode_b(op->opcode,op->funct3,r1,r2,0);
            break;
        }
        int64_t off=target-(int64_t)line->address;
        word=encode_b(op->opcode,op->funct3,r1,r2,(int32_t)off); break;
    }
    case OPSTYLE_RD_LABEL: {
        if(count!=2){errors_add(errors,line->line_no,"%s expects rd,label",line->mnemonic);return false;}
        int rd=parse_register(ops_arr[0]);
        if(rd<0){errors_add(errors,line->line_no,"Invalid register.");return false;}
        int64_t target=0; bool sym=false;
        if(!eval_expr(ops_arr[1],symbols,&target,&sym)){
            /* Extern JAL target - create relocation */
            char name[MAX_LABEL_LEN+1]; strncpy(name,ops_arr[1],MAX_LABEL_LEN); name[MAX_LABEL_LEN]='\0'; trim(name);
            RelocationEntry rel; memset(&rel,0,sizeof(rel));
            rel.offset = line->address;
            rel.type = RELOC_R_RISCV_JAL;
            snprintf(rel.symbol_name, sizeof(rel.symbol_name), "%s", name);
            reloc_list_push(relocs, &rel);
            word=encode_j(op->opcode,rd,0);
            break;
        }
        int64_t off=target-(int64_t)line->address;
        word=encode_j(op->opcode,rd,(int32_t)off); break;
    }
    case OPSTYLE_RD_UIMM: {
        if(count!=2){errors_add(errors,line->line_no,"%s expects rd,imm20",line->mnemonic);return false;}
        int rd=parse_register(ops_arr[0]); int64_t imm=0; bool sym=false;
        if(rd<0){errors_add(errors,line->line_no,"Invalid register.");return false;}
        if(!eval_expr(ops_arr[1],symbols,&imm,&sym)){
            char name[MAX_LABEL_LEN+1]; strncpy(name,ops_arr[1],MAX_LABEL_LEN); name[MAX_LABEL_LEN]='\0'; trim(name);
            RelocationEntry rel; memset(&rel,0,sizeof(rel));
            rel.offset = line->address;
            rel.type = RELOC_R_RISCV_HI20;
            snprintf(rel.symbol_name, sizeof(rel.symbol_name), "%s", name);
            reloc_list_push(relocs, &rel);
            imm = 0;
        }
        uint32_t shifted=((uint32_t)imm&0xFFFFFu)<<12;
        word=encode_u(op->opcode,rd,(int32_t)shifted); break;
    }
    case OPSTYLE_NO_OPERANDS: {
        if(!is_blank_operands(line->operands)){errors_add(errors,line->line_no,"%s no operands",line->mnemonic);return false;}
        if(strcmp(line->mnemonic,"ecall")==0) word=0x00000073u;
        else if(strcmp(line->mnemonic,"ebreak")==0) word=0x00100073u;
        else {errors_add(errors,line->line_no,"Unsupported: %s",line->mnemonic);return false;}
        break;
    }
    default: errors_add(errors,line->line_no,"Unsupported style"); return false;
    }

    line->bytes[0]=(uint8_t)(word&0xFF); line->bytes[1]=(uint8_t)((word>>8)&0xFF);
    line->bytes[2]=(uint8_t)((word>>16)&0xFF); line->bytes[3]=(uint8_t)((word>>24)&0xFF);
    line->emitted_size=4; line->emits_bytes=true;
    return true;
}

/* ---- Assemble to Object File (for linker) ---- */
bool assemble_to_object(LineVector *lines, SymbolTable *symbols, ObjectFile *obj, ErrorList *errors)
{
    uint32_t program_end = 0;
    if (!run_pass1(lines, symbols, errors, &program_end)) return false;

    /* Object-file pass2: use encode_instruction_obj which tolerates externs */
    size_t img_size = (program_end + 0x1000u > 0x100000u) ? (size_t)(program_end + 0x1000u) : 0x100000u;
    MemoryImage image;
    memory_image_init(&image, img_size);

    for (size_t i = 0; i < lines->count; ++i) {
        ParsedLine *line = &lines->items[i];
        if (line->is_empty || line->mnemonic[0]=='\0') continue;
        if (line->is_directive) {
            if (strcmp(line->mnemonic,".end")==0) break;
            if (!emit_directive_pass2(line, symbols, errors)) {
                memory_image_free(&image); return false;
            }
        } else {
            RelocationList *rl = (line->section == SEC_TEXT) ? &obj->text_relocs : &obj->data_relocs;
            if (!encode_instruction_obj(line, symbols, rl, errors)) {
                memory_image_free(&image); return false;
            }
        }
        if (line->emits_bytes) {
            for (size_t j=0; j<line->emitted_size; ++j) {
                uint32_t addr = line->address + (uint32_t)j;
                memory_write_byte(&image, addr, line->bytes[j]);
            }
        }
    }

    /* Determine text and data ranges */
    uint32_t text_min=UINT32_MAX, text_max=0, data_min=UINT32_MAX, data_max=0;
    for (size_t i=0; i<lines->count; ++i) {
        ParsedLine *line = &lines->items[i];
        if (!line->emits_bytes || line->emitted_size==0) continue;
        uint32_t lo=line->address, hi=line->address+(uint32_t)line->emitted_size-1;
        if (line->section==SEC_TEXT) {
            if (lo < text_min) text_min = lo;
            if (hi > text_max) text_max = hi;
        } else {
            if (lo < data_min) data_min = lo;
            if (hi > data_max) data_max = hi;
        }
    }

    /* Copy text section data */
    if (text_min<=text_max) {
        obj->text.base_address = text_min;
        obj->text.size = text_max - text_min + 1;
        obj->text.data = malloc(obj->text.size);
        if (!obj->text.data) { memory_image_free(&image); return false; }
        memcpy(obj->text.data, image.data + text_min, obj->text.size);
    }

    /* Copy data section data */
    if (data_min<=data_max) {
        obj->data.base_address = data_min;
        obj->data.size = data_max - data_min + 1;
        obj->data.data = malloc(obj->data.size);
        if (!obj->data.data) { memory_image_free(&image); return false; }
        memcpy(obj->data.data, image.data + data_min, obj->data.size);
    }

    /* Copy symbols */
    for (size_t i=0; i<symbols->capacity; i++) {
        if (!symbols->items[i].occupied) continue;
        Symbol *src = &symbols->items[i];
        symbol_table_add(&obj->symbols, src->name, src->address, src->section, src->is_absolute);
        Symbol *dst = symbol_table_find(&obj->symbols, src->name);
        if (dst) {
            dst->binding = src->binding;
            dst->defined = src->defined;
        }
    }

    memory_image_free(&image);
    return true;
}
