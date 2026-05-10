# RV32I Assembler + Linker

**PicoRV32 uyumlu C implementasyonu / PicoRV32-compatible C implementation**

Bu proje, RV32I assembly dosyalarını object dosyalarına çeviren bir assembler ve bu object dosyalarını FPGA/PicoRV32 için `.hex`, `.mem`, `.bin` ve `.map` çıktılarına dönüştüren bir linker içerir.

This project contains an assembler that converts RV32I assembly files into object files and a linker that produces `.hex`, `.mem`, `.bin`, and `.map` outputs for FPGA/PicoRV32 usage.

---

## İçindekiler / Table of Contents

- [Türkçe Açıklama](#türkçe-açıklama)
  - [Proje Yapısı](#proje-yapısı)
  - [Linux Üzerinde Çalıştırma](#linux-üzerinde-çalıştırma)
  - [Windows Üzerinde Çalıştırma](#windows-üzerinde-çalıştırma)
  - [Manuel Kullanım](#manuel-kullanım)
  - [Testleri Çalıştırma](#testleri-çalıştırma)
  - [GUI Kullanımı](#gui-kullanımı)
  - [Desteklenen Komutlar](#desteklenen-komutlar-rv32i)
  - [Object Dosya Formatı](#object-dosya-formatı-o)
  - [Relocation Tipleri](#relocation-tipleri)
  - [Bellek Haritası](#bellek-haritası-picorv32)
  - [FPGA Yükleme](#fpga-yükleme)
- [English Documentation](#english-documentation)
  - [Project Structure](#project-structure)
  - [Running on Linux](#running-on-linux)
  - [Running on Windows](#running-on-windows)
  - [Manual Usage](#manual-usage)
  - [Running Tests](#running-tests)
  - [GUI Usage](#gui-usage)
  - [Supported Instructions](#supported-instructions-rv32i)
  - [Object File Format](#object-file-format-o)
  - [Relocation Types](#relocation-types)
  - [Memory Map](#memory-map-picorv32)
  - [FPGA Loading](#fpga-loading)

---

# Türkçe Açıklama

## Proje Yapısı

```text
rv32i_c/
├── src/
│   ├── assembler/       # Assembler parser/encoder/main dosyaları
│   ├── linker/          # Linker parser/relocator/writer/main dosyaları
│   └── common/          # Ortak yardımcı fonksiyonlar
├── tests/
│   ├── run_tests.py
│   └── test_programs/   # Otomatik test assembly dosyaları
├── test_programs/
│   ├── main.s           # Knight Rider ana programı (_start)
│   └── utils.s          # Delay fonksiyonu + .data bölümü
├── gui/                 # Flask tabanlı web arayüzü
├── fpga/                # FPGA kaynakları ve yardımcı dosyalar
├── output/              # Build sonrasında oluşan çıktı dosyaları
├── Makefile
├── build.sh
├── launch_app.sh
├── requirements.txt
└── README.md
```

## Platform Desteği

Proje hem Linux hem de Windows üzerinde çalıştırılabilir. Ancak proje dosyalarında `Makefile` ve `.sh` betikleri bulunduğu için en sorunsuz kullanım Linux, WSL veya MSYS2/Git Bash gibi Unix benzeri terminal ortamlarıdır.

| Platform | Durum | Önerilen Yöntem |
|---|---|---|
| Linux | Desteklenir | `make`, `gcc`, `python3` |
| Windows + WSL | Desteklenir | Ubuntu/WSL içinde Linux komutları |
| Windows + MSYS2/Git Bash | Desteklenir | `make`, `gcc`, `python` |
| Windows PowerShell/CMD | Kısmen desteklenir | Elle `gcc` komutları veya WSL/MSYS2 önerilir |

## Linux Üzerinde Çalıştırma

### 1. Gerekli paketleri kurun

Debian/Ubuntu tabanlı sistemlerde:

```bash
sudo apt update
sudo apt install build-essential python3 python3-venv python3-pip make
```

Fedora tabanlı sistemlerde:

```bash
sudo dnf install gcc make python3 python3-pip
```

Arch tabanlı sistemlerde:

```bash
sudo pacman -S gcc make python python-pip
```

### 2. Projeyi derleyin

Tek komutla tüm build işlemini yapmak için:

```bash
bash build.sh
```

Alternatif olarak:

```bash
make clean
make build
```

Build sonrasında çıktılar `output/` klasöründe oluşur:

```text
output/knight_rider.hex
output/knight_rider.mem
output/knight_rider.bin
output/knight_rider.map
```

### 3. Testleri çalıştırın

```bash
make test
```

veya:

```bash
python3 tests/run_tests.py
```

### 4. GUI'yi çalıştırın

```bash
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
python gui/app.py
```

Ardından tarayıcıdan şu adrese gidin:

```text
http://localhost:5000
```

Alternatif olarak Linux üzerinde başlatıcı betiği kullanabilirsiniz:

```bash
bash launch_app.sh
```

## Windows Üzerinde Çalıştırma

Windows için iki önerilen yöntem vardır.

### Yöntem 1: WSL ile çalıştırma önerilir

WSL, Windows üzerinde Linux ortamı sağlar. Bu proje için en sorunsuz Windows yöntemi WSL kullanmaktır.

#### 1. WSL kurun

PowerShell'i yönetici olarak açın ve çalıştırın:

```powershell
wsl --install
```

Bilgisayarı yeniden başlattıktan sonra Ubuntu terminalini açın.

#### 2. Ubuntu/WSL içinde paketleri kurun

```bash
sudo apt update
sudo apt install build-essential python3 python3-venv python3-pip make
```

#### 3. Proje klasörüne girin

Örneğin proje Windows'ta `C:\Users\Kullanici\Desktop\rv32i_c` konumundaysa WSL içinde şu şekilde girilir:

```bash
cd /mnt/c/Users/Kullanici/Desktop/rv32i_c
```

#### 4. Build alın

```bash
bash build.sh
```

veya:

```bash
make clean
make build
```

#### 5. Testleri çalıştırın

```bash
make test
```

#### 6. GUI'yi çalıştırın

```bash
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
python gui/app.py
```

Tarayıcıdan açın:

```text
http://localhost:5000
```

### Yöntem 2: MSYS2 / Git Bash ile çalıştırma

Native Windows üzerinde Unix benzeri terminal kullanmak için MSYS2 önerilir.

#### 1. MSYS2 kurun

MSYS2 kurulduktan sonra **MSYS2 UCRT64** terminalini açın.

#### 2. Gerekli paketleri kurun

```bash
pacman -Syu
pacman -S --needed mingw-w64-ucrt-x86_64-gcc make python python-pip
```

Güncelleme sonrası terminal kapanırsa tekrar **MSYS2 UCRT64** terminalini açın.

#### 3. Proje klasörüne girin

Örnek:

```bash
cd /c/Users/Kullanici/Desktop/rv32i_c
```

#### 4. Build alın

```bash
make clean
make build
```

veya shell betiğiyle:

```bash
bash build.sh
```

#### 5. Testleri çalıştırın

```bash
make test
```

Eğer sisteminizde `python3` yerine `python` komutu varsa şu komutu da kullanabilirsiniz:

```bash
python tests/run_tests.py
```

#### 6. GUI'yi çalıştırın

```bash
python -m venv venv
source venv/Scripts/activate
pip install -r requirements.txt
python gui/app.py
```

Tarayıcıdan açın:

```text
http://localhost:5000
```

### PowerShell/CMD Notu

PowerShell veya CMD üzerinde `bash build.sh`, `make`, `./assembler_bin` gibi Unix tarzı komutlar doğrudan çalışmayabilir. Bu yüzden Windows için WSL veya MSYS2 kullanılması önerilir.

Yine de PowerShell üzerinde manuel derleme yapmak isterseniz GCC/MinGW kurulu olmalıdır. Örnek assembler derleme komutu:

```powershell
gcc -Wall -Wextra -O2 -std=c99 -I src/common -I src/assembler -I src/linker -o assembler_bin.exe src/common/utils.c src/assembler/encoder.c src/assembler/parser.c src/assembler/main.c
```

Örnek linker derleme komutu:

```powershell
gcc -Wall -Wextra -O2 -std=c99 -I src/common -I src/assembler -I src/linker -o linker_bin.exe src/linker/core.c src/linker/parser.c src/linker/relocator.c src/linker/writer.c src/linker/main.c
```

Örnek programı üretmek için:

```powershell
mkdir output
.\assembler_bin.exe test_programs\main.s test_programs\utils.s
.\linker_bin.exe test_programs\main.o test_programs\utils.o -o output\knight_rider --text-base 00000000 --data-base 00010000 --stack-top 00020000
```

## Manuel Kullanım

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

## Testleri Çalıştırma

```bash
make all
python3 tests/run_tests.py
```

Testler assembler/linker çıkış durumuna ek olarak string direktiflerini, `.data` yerleşimini ve ABS32 data relocation sonucunu kontrol eder.

## GUI Kullanımı

GUI, Flask tabanlı bir web arayüzüdür.

```bash
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
python gui/app.py
```

Windows MSYS2/Git Bash üzerinde:

```bash
python -m venv venv
source venv/Scripts/activate
pip install -r requirements.txt
python gui/app.py
```

Adres:

```text
http://localhost:5000
```

## Desteklenen Komutlar (RV32I)

```text
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
```

## Object Dosya Formatı (.o)

Object dosyaları JSON formatındadır ve şu alanları içerir:

- `filename`: kaynak dosya adı
- `text`: 32-bit instruction kelimeleri, `uint` dizisi
- `data`: data bölümü, byte dizisi
- `symbols`: sembol tablosu `{isim: {section, offset, global}}`
- `relocations`: relocation kayıtları
- `globals`: global sembol isimleri

## Relocation Tipleri

| Tip | Açıklama |
|---|---|
| `BRANCH` | B-type PC-relative dal |
| `JAL` | J-type PC-relative atlama |
| `CALL` | `auipc + jalr` çifti, `call` pseudo |
| `LA` | `auipc + addi` çifti, `la` pseudo |
| `LI` | `lui + addi` çifti, büyük sabit |
| `HI20` | U-type upper 20 bit |
| `LO12` | I-type lower 12 bit |
| `PCREL_HI20` | `auipc` için upper 20 bit |
| `ABS32` | `.data` bölümünde 32-bit mutlak adres |

## Bellek Haritası (PicoRV32)

```text
0x00000000 - 0x0000FFFF : TEXT (BRAM, program kodu)
0x00010000 - 0x0001FFFF : DATA/BSS (RAM)
0x00020000              : STACK_TOP
0x10000000              : GPIO / LED (memory-mapped)
```

## FPGA Yükleme

`knight_rider.mem` dosyasını Verilog içinde kullanabilirsiniz:

```verilog
reg [31:0] bram [0:16383]; // 64 KB BRAM
initial $readmemh("knight_rider.mem", bram);
```

`.mem` dosyası `$readmemh` adres direktifleri (`@...`) kullanarak TEXT ve DATA bölümlerini ilgili word adreslerine yerleştirir.

LED GPIO yazmacı `0x10000000` adresindedir. `main.s` içindeki aşağıdaki komut LED yazmacına veri yazar:

```asm
sw t2, 0(t1)
```

---

# English Documentation

## Project Structure

```text
rv32i_c/
├── src/
│   ├── assembler/       # Assembler parser/encoder/main files
│   ├── linker/          # Linker parser/relocator/writer/main files
│   └── common/          # Shared helper functions
├── tests/
│   ├── run_tests.py
│   └── test_programs/   # Automated test assembly files
├── test_programs/
│   ├── main.s           # Knight Rider main program (_start)
│   └── utils.s          # Delay function + .data section
├── gui/                 # Flask-based web interface
├── fpga/                # FPGA sources and helper files
├── output/              # Generated output files after build
├── Makefile
├── build.sh
├── launch_app.sh
├── requirements.txt
└── README.md
```

## Platform Support

The project can run on both Linux and Windows. Since the project includes a `Makefile` and `.sh` scripts, the easiest workflow is Linux, WSL, or a Unix-like terminal environment such as MSYS2/Git Bash.

| Platform | Status | Recommended Method |
|---|---|---|
| Linux | Supported | `make`, `gcc`, `python3` |
| Windows + WSL | Supported | Linux commands inside Ubuntu/WSL |
| Windows + MSYS2/Git Bash | Supported | `make`, `gcc`, `python` |
| Windows PowerShell/CMD | Partially supported | Manual `gcc` commands or WSL/MSYS2 recommended |

## Running on Linux

### 1. Install required packages

On Debian/Ubuntu-based systems:

```bash
sudo apt update
sudo apt install build-essential python3 python3-venv python3-pip make
```

On Fedora-based systems:

```bash
sudo dnf install gcc make python3 python3-pip
```

On Arch-based systems:

```bash
sudo pacman -S gcc make python python-pip
```

### 2. Build the project

To build everything with one command:

```bash
bash build.sh
```

Alternatively:

```bash
make clean
make build
```

After the build, generated files will be placed under `output/`:

```text
output/knight_rider.hex
output/knight_rider.mem
output/knight_rider.bin
output/knight_rider.map
```

### 3. Run tests

```bash
make test
```

or:

```bash
python3 tests/run_tests.py
```

### 4. Run the GUI

```bash
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
python gui/app.py
```

Then open this address in your browser:

```text
http://localhost:5000
```

On Linux, you can also use the launcher script:

```bash
bash launch_app.sh
```

## Running on Windows

There are two recommended ways to run the project on Windows.

### Method 1: Using WSL, recommended

WSL provides a Linux environment on Windows. This is the most reliable Windows workflow for this project.

#### 1. Install WSL

Open PowerShell as Administrator and run:

```powershell
wsl --install
```

Restart your computer if required, then open the Ubuntu terminal.

#### 2. Install packages inside Ubuntu/WSL

```bash
sudo apt update
sudo apt install build-essential python3 python3-venv python3-pip make
```

#### 3. Go to the project directory

For example, if the project is located at `C:\Users\User\Desktop\rv32i_c` on Windows, use this path inside WSL:

```bash
cd /mnt/c/Users/User/Desktop/rv32i_c
```

#### 4. Build the project

```bash
bash build.sh
```

or:

```bash
make clean
make build
```

#### 5. Run tests

```bash
make test
```

#### 6. Run the GUI

```bash
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
python gui/app.py
```

Open in your browser:

```text
http://localhost:5000
```

### Method 2: Using MSYS2 / Git Bash

MSYS2 is recommended if you want to run the project more natively on Windows while still having Unix-like commands.

#### 1. Install MSYS2

After installation, open the **MSYS2 UCRT64** terminal.

#### 2. Install required packages

```bash
pacman -Syu
pacman -S --needed mingw-w64-ucrt-x86_64-gcc make python python-pip
```

If the terminal closes after the update, reopen **MSYS2 UCRT64**.

#### 3. Go to the project directory

Example:

```bash
cd /c/Users/User/Desktop/rv32i_c
```

#### 4. Build the project

```bash
make clean
make build
```

or with the shell script:

```bash
bash build.sh
```

#### 5. Run tests

```bash
make test
```

If your environment uses `python` instead of `python3`, you can also run:

```bash
python tests/run_tests.py
```

#### 6. Run the GUI

```bash
python -m venv venv
source venv/Scripts/activate
pip install -r requirements.txt
python gui/app.py
```

Open in your browser:

```text
http://localhost:5000
```

### PowerShell/CMD Note

Unix-style commands such as `bash build.sh`, `make`, and `./assembler_bin` may not work directly in PowerShell or CMD. For that reason, WSL or MSYS2 is recommended on Windows.

If you still want to build manually in PowerShell, GCC/MinGW must be installed. Example assembler build command:

```powershell
gcc -Wall -Wextra -O2 -std=c99 -I src/common -I src/assembler -I src/linker -o assembler_bin.exe src/common/utils.c src/assembler/encoder.c src/assembler/parser.c src/assembler/main.c
```

Example linker build command:

```powershell
gcc -Wall -Wextra -O2 -std=c99 -I src/common -I src/assembler -I src/linker -o linker_bin.exe src/linker/core.c src/linker/parser.c src/linker/relocator.c src/linker/writer.c src/linker/main.c
```

To build the sample program:

```powershell
mkdir output
.\assembler_bin.exe test_programs\main.s test_programs\utils.s
.\linker_bin.exe test_programs\main.o test_programs\utils.o -o output\knight_rider --text-base 00000000 --data-base 00010000 --stack-top 00020000
```

## Manual Usage

```bash
# Compile
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

## Running Tests

```bash
make all
python3 tests/run_tests.py
```

The tests verify assembler/linker exit status, string directives, `.data` placement, and ABS32 data relocation results.

## GUI Usage

The GUI is a Flask-based web interface.

```bash
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
python gui/app.py
```

On Windows with MSYS2/Git Bash:

```bash
python -m venv venv
source venv/Scripts/activate
pip install -r requirements.txt
python gui/app.py
```

URL:

```text
http://localhost:5000
```

## Supported Instructions (RV32I)

```text
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
```

## Object File Format (.o)

Object files are stored in JSON format and contain the following fields:

- `filename`: source file name
- `text`: 32-bit instruction words as a `uint` array
- `data`: data section as a byte array
- `symbols`: symbol table `{name: {section, offset, global}}`
- `relocations`: relocation records
- `globals`: global symbol names

## Relocation Types

| Type | Description |
|---|---|
| `BRANCH` | B-type PC-relative branch |
| `JAL` | J-type PC-relative jump |
| `CALL` | `auipc + jalr` pair for the `call` pseudo-instruction |
| `LA` | `auipc + addi` pair for the `la` pseudo-instruction |
| `LI` | `lui + addi` pair for large immediates |
| `HI20` | U-type upper 20 bits |
| `LO12` | I-type lower 12 bits |
| `PCREL_HI20` | Upper 20 bits for `auipc` |
| `ABS32` | 32-bit absolute address in the `.data` section |

## Memory Map (PicoRV32)

```text
0x00000000 - 0x0000FFFF : TEXT (BRAM, program code)
0x00010000 - 0x0001FFFF : DATA/BSS (RAM)
0x00020000              : STACK_TOP
0x10000000              : GPIO / LED (memory-mapped)
```

## FPGA Loading

Use the `knight_rider.mem` file in Verilog:

```verilog
reg [31:0] bram [0:16383]; // 64 KB BRAM
initial $readmemh("knight_rider.mem", bram);
```

The `.mem` file uses `$readmemh` address directives (`@...`) to place TEXT and DATA sections at the correct word addresses.

The LED GPIO register is located at address `0x10000000`. The following instruction in `main.s` writes to the LED register:

```asm
sw t2, 0(t1)
```
