#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_LABEL_LEN 63
#define MAX_MNEMONIC_LEN 31
#define MAX_OPERAND_TEXT 255
#define MAX_LINE_TEXT 511
#define MAX_ERRORS 256
#define INITIAL_CAPACITY 64

typedef enum {
    SEC_TEXT = 0,
    SEC_DATA = 1
} Section;

typedef enum {
    FMT_R = 0,
    FMT_I,
    FMT_S,
    FMT_B,
    FMT_U,
    FMT_J,
    FMT_SYS,
    FMT_INVALID
} InstrFormat;

typedef enum {
    OPSTYLE_NONE = 0,
    OPSTYLE_RD_RS1_RS2,
    OPSTYLE_RD_RS1_IMM,
    OPSTYLE_RD_IMM_RS1,   /* loads/jalr: rd, imm(rs1) */
    OPSTYLE_RS2_IMM_RS1,  /* stores: rs2, imm(rs1) */
    OPSTYLE_RS1_RS2_LABEL,
    OPSTYLE_RD_LABEL,
    OPSTYLE_RD_UIMM,
    OPSTYLE_NO_OPERANDS
} OperandStyle;

/* ---- Symbol binding for linker ---- */
typedef enum {
    BIND_LOCAL  = 0,
    BIND_GLOBAL = 1,
    BIND_EXTERN = 2
} SymbolBinding;

typedef struct {
    char name[MAX_LABEL_LEN + 1];
    uint32_t address;
    Section section;
    bool defined;
    bool is_absolute;
    bool occupied;
    SymbolBinding binding;
} Symbol;

typedef struct {
    Symbol *items;
    size_t count;
    size_t capacity;
} SymbolTable;

typedef struct {
    char label[MAX_LABEL_LEN + 1];
    char mnemonic[MAX_MNEMONIC_LEN + 1];
    char operands[MAX_OPERAND_TEXT + 1];
    char original[MAX_LINE_TEXT + 1];
    int line_no;
    uint32_t address;
    Section section;
    bool is_empty;
    bool is_directive;
    bool emits_bytes;
    size_t emitted_size;
    uint8_t bytes[256];
} ParsedLine;

typedef struct {
    ParsedLine *items;
    size_t count;
    size_t capacity;
} LineVector;

typedef struct {
    char messages[MAX_ERRORS][256];
    size_t count;
} ErrorList;

typedef struct {
    uint8_t *data;
    bool *used;
    size_t size;
} MemoryImage;

/* ---- Relocation types (RISC-V subset) ---- */
typedef enum {
    RELOC_R_RISCV_JAL    = 1,   /* JAL offset (J-type) */
    RELOC_R_RISCV_BRANCH = 2,   /* Branch offset (B-type) */
    RELOC_R_RISCV_HI20   = 3,   /* LUI/AUIPC upper 20 bits */
    RELOC_R_RISCV_LO12_I = 4,   /* ADDI/Load lower 12 bits (I-type) */
    RELOC_R_RISCV_LO12_S = 5,   /* Store lower 12 bits (S-type) */
    RELOC_R_RISCV_32     = 6    /* .word absolute 32-bit */
} RelocationType;

typedef struct {
    uint32_t offset;                    /* Position within section */
    RelocationType type;                /* Relocation type */
    char symbol_name[MAX_LABEL_LEN + 1];
    int64_t addend;                     /* Extra offset value */
} RelocationEntry;

typedef struct {
    RelocationEntry *items;
    size_t count;
    size_t capacity;
} RelocationList;

/* ---- Section data for object files ---- */
typedef struct {
    char name[16];              /* ".text" or ".data" */
    uint32_t base_address;
    uint32_t size;
    uint8_t *data;
} SectionData;

/* ---- Object file representation ---- */
typedef struct {
    SectionData text;
    SectionData data;
    SymbolTable symbols;
    RelocationList text_relocs;
    RelocationList data_relocs;
    char source_name[256];
} ObjectFile;

#endif
