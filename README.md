# PicoRV32 RV32I Assembler (C99)

Bu proje, PicoRV32 üzerinde kullanılabilecek RV32I alt kümesi için iki geçişli (two-pass) bir assembler gerçekleştirir.

## Özellikler

- C99 ile yazılmış modüler yapı
- Two-pass assembler mimarisi
- Desteklenen formatlar: R, I, S, B, U, J
- Ek sistem komutları: `ecall`, `ebreak`
- Desteklenen direktifler: `.text`, `.data`, `.word`, `.byte`, `.org`, `.end`
- Çıktı formatı: Intel HEX
- İsteğe bağlı listing (`.lst`) üretimi
- Register isimleri: `x0..x31` ve ABI alias'ları (`zero`, `ra`, `sp`, `a0` vb.)
- İfadeler: sayı, karakter literal'i, `label`, `label+imm`, `label-imm`

## Desteklenen komutlar

### R-type
`add sub sll slt sltu xor srl sra or and`

### I-type
`addi slti sltiu xori ori andi slli srli srai`

### Load/JALR
`lb lh lw lbu lhu jalr`

### S-type
`sb sh sw`

### B-type
`beq bne blt bge bltu bgeu`

### U-type
`lui auipc`

### J-type
`jal`

### System
`ecall ebreak`

## Derleme

```bash
make
```

## Kullanım

```bash
./build/picorv32asm input.s output.hex [listing.lst]
```

Örnek:

```bash
./build/picorv32asm tests/test1_counter.s build/test1.hex build/test1.lst
```

## Dizin yapısı

- `include/` : başlık dosyaları
- `src/` : kaynak kodlar
- `tests/` : örnek test assembly programları
- `build/` : derlenmiş ikili ve test çıktıları

## Tasarım notları

- Pass 1 aşamasında satırlar ayrıştırılır, adresler atanır ve label'lar sembol tablosuna yerleştirilir.
- Pass 2 aşamasında komutlar 32-bit makine koduna çevrilir ve bellek imgesi oluşturulur.
- Sembol tablosu açık adreslemeli hash tablo olarak gerçekleştirilmiştir.
- Opcode tablosu sabit ve küçük olduğu için salt-okunur statik dizi yapısı kullanılmıştır.
- Bellek çıktısı little-endian olarak Intel HEX kaydına dönüştürülür.
