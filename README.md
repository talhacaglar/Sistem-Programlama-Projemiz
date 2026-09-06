# RV32I Assembler & Linker

[English](#english) · [Türkçe](#türkçe)

## English

An assembler and linker written in C for RV32I programs, with PicoRV32-oriented memory output and a Flask interface.

### Features

- Assembly parsing and instruction encoding, object files and symbol resolution.
- Relocation handling and configurable text/data/stack addresses.
- Command-line build, regression programs and a local web interface.

### Getting started

Use the repository build script with a C toolchain and Make installed.

```bash
git clone https://github.com/talhacaglar/rv32i-assembler-linker.git
cd rv32i-assembler-linker
bash build.sh
python3 tests/run_tests.py
```

For the GUI: create a virtual environment, install `requirements.txt`, then run `python gui/app.py`. See the reference for supported instructions, object format, relocation types, memory map and FPGA integration.

[Detailed technical reference](REFERENCE.md)

## Türkçe

RV32I programları için C ile yazılmış assembler ve linker; PicoRV32 odaklı bellek çıktıları ve Flask arayüzü sunar.

### Özellikler

- Assembly ayrıştırma ve komut kodlama, object dosyaları ve sembol çözümleme.
- Relocation işleme ve ayarlanabilir text/data/stack adresleri.
- Komut satırı derlemesi, regresyon programları ve yerel web arayüzü.

### Başlangıç

C araç zinciri ve Make kurulu bir ortamda deponun derleme betiğini kullanın.

```bash
git clone https://github.com/talhacaglar/rv32i-assembler-linker.git
cd rv32i-assembler-linker
bash build.sh
python3 tests/run_tests.py
```

GUI için sanal ortam oluşturun, `requirements.txt` bağımlılıklarını kurun ve `python gui/app.py` çalıştırın. Desteklenen komutlar, object biçimi, relocation türleri, bellek haritası ve FPGA entegrasyonu referanstadır.

[Ayrıntılı teknik referans](REFERENCE.md)
