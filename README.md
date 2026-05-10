# RV32I Assembler + Linker (C implementasyonu)
# PicoRV32 uyumlu

## Proje Yapisi

```
rv32i_c/
├── src/
│   ├── assembler/       # Assembler parser/encoder/main
│   ├── linker/          # Linker parser/relocator/writer/main
│   └── common/          # Ortak yardimci fonksiyonlar
├── tests/
│   ├── run_tests.py
│   └── test_programs/   # Otomatik test assembly dosyalari
├── test_programs/
│   ├── main.s           # Knight Rider ana program (_start)
│   └── utils.s          # Delay fonksiyonu + .data bolumu
├── gui/                 # Flask tabanli web arayuzu
├── output/              # Uretilen dosyalar
│   ├── knight_rider.hex
│   ├── knight_rider.mem
│   ├── knight_rider.bin
│   └── knight_rider.map
├── Makefile
├── build.sh
├── requirements.txt
└── README.md
```

## Tek Komutla Build

```bash
bash build.sh
```

## Manuel Kullanim

```bash
# Derle
make all

# Assemble
cd test_programs
../assembler_bin main.s utils.s

# Link
../linker_bin main.o utils.o \
    -o ../output/knight_rider \
    --text-base 00000000 \
    --data-base 00010000 \
    --stack-top 00020000
```

## Test

```bash
make all
python3 tests/run_tests.py
```

Testler assembler/linker cikis durumuna ek olarak string direktiflerini,
`.data` yerlesimini ve ABS32 data relocation sonucunu da kontrol eder.

## GUI

```bash
python3 -m venv venv
./venv/bin/pip install -r requirements.txt
./venv/bin/python gui/app.py
```

## Desteklenen Komutlar (RV32I)

R-type : add, sub, sll, slt, sltu, xor, srl, sra, or, and
I-type : addi, slti, sltiu, xori, ori, andi, slli, srli, srai
Load   : lw, lh, lb, lhu, lbu
Store  : sw, sh, sb
Branch : beq, bne, blt, bge, bltu, bgeu
Upper  : lui, auipc
Jump   : jal, jalr
System : ecall, ebreak, fence
Pseudo : nop, li, mv, la, call, ret, j, not, neg,
         beqz, bnez, blez, bgez, bltz, bgtz, seqz, snez

## Object Dosya Formati (.o)

JSON formatinda, su alanlari icerir:
- filename  : kaynak dosya adi
- text      : 32-bit instruction kelimeleri (uint dizisi)
- data      : data bolumu (byte dizisi)
- symbols   : sembol tablosu {isim: {section, offset, global}}
- relocations : relocation kayitlari
- globals   : global sembol isimleri

## Relocation Tipleri

BRANCH     : B-type PC-relative dal
JAL        : J-type PC-relative atlama
CALL       : auipc+jalr cifti (call pseudo)
LA         : auipc+addi cifti (la pseudo)
LI         : lui+addi cifti (buyuk sabite)
HI20       : U-type upper 20 bit
LO12       : I-type lower 12 bit
PCREL_HI20 : auipc icin upper 20 bit
ABS32      : data section'da 32-bit mutlak adres

## Bellek Haritasi (PicoRV32)

0x00000000 - 0x0000FFFF : TEXT (BRAM, program kodu)
0x00010000 - 0x0001FFFF : DATA/BSS (RAM)
0x00020000              : STACK_TOP
0x10000000              : GPIO / LED (memory-mapped)

## FPGA Yukleme

knight_rider.mem dosyasini Verilog'da kullan:

```verilog
reg [31:0] bram [0:16383]; // 64KB BRAM
initial $readmemh("knight_rider.mem", bram);
```

`.mem` dosyasi `$readmemh` adres direktifleri (`@...`) kullanarak TEXT ve
DATA bolumlerini ilgili word adreslerine yerlestirilir.

LED GPIO yazmaci 0x10000000 adresinde,
main.s'deki `sw t2, 0(t1)` komutuyla yazılır.
