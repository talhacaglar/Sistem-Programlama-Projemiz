CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -O2 -Iinclude

# Common object files (shared between assembler and linker)
COMMON_OBJ = src/utils.o src/objfile.o src/hex_writer.o

# Assembler
ASM_OBJ = src/assembler.o src/parser.o src/opcodes.o src/main.o $(COMMON_OBJ)
ASM_TARGET = build/picorv32asm

# Linker
LINK_OBJ = src/linker.o src/linker_script.o src/linker_main.o $(COMMON_OBJ)
LINK_TARGET = build/picorv32link

all: $(ASM_TARGET) $(LINK_TARGET)

$(ASM_TARGET): $(ASM_OBJ) | build
	$(CC) $(CFLAGS) -o $@ $(ASM_OBJ)

$(LINK_TARGET): $(LINK_OBJ) | build
	$(CC) $(CFLAGS) -o $@ $(LINK_OBJ)

build:
	mkdir -p build

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o build/*

# ---- Demo: multi-file assemble + link for Tang Nano 9K ----
demo: $(ASM_TARGET) $(LINK_TARGET)
	@echo "=== Assembling test files ==="
	./$(ASM_TARGET) -c tests/linker/main.s -o build/main.o
	./$(ASM_TARGET) -c tests/linker/led.s -o build/led.o
	./$(ASM_TARGET) -c tests/linker/utils.s -o build/utils.o
	@echo ""
	@echo "=== Dumping object files ==="
	./$(ASM_TARGET) --dump build/main.o
	@echo ""
	./$(ASM_TARGET) --dump build/led.o
	@echo ""
	./$(ASM_TARGET) --dump build/utils.o
	@echo ""
	@echo "=== Linking ==="
	./$(LINK_TARGET) -T tests/linker/tangnano9k.ld -o build/firmware.hex build/main.o build/led.o build/utils.o
	@echo ""
	@echo "=== Converting for FPGA ==="
	python3 fpga/scripts/hex2mem.py build/firmware.hex fpga/mem/firmware.hex
	@echo "Done!"

# ---- Legacy single-file test ----
test: $(ASM_TARGET)
	./$(ASM_TARGET) tests/test1_counter.s build/test1.hex build/test1.lst
	./$(ASM_TARGET) tests/test2_memory.s build/test2.hex
	./$(ASM_TARGET) tests/test3_call.s build/test3.hex

.PHONY: all clean demo test
