#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INPUT_MEM = ROOT / "output" / "knight_rider.mem"
OUT_DIR = ROOT / "fpga" / "mem"
TEXT_OUT = OUT_DIR / "firmware_text.mem"
DATA_OUT = OUT_DIR / "firmware_data.mem"

DATA_WORD_BASE = 0x00010000 // 4


def parse_words(path):
    current_addr = 0
    words = {}

    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("//"):
            continue
        if line.startswith("@"):
            current_addr = int(line[1:], 16)
            continue
        words[current_addr] = line
        current_addr += 1

    return words


def write_sparse_mem(path, words):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as f:
        if not words:
            f.write("@00000000\n")
            f.write("00000013\n")
            return

        current = None
        for addr in sorted(words):
            if current != addr:
                f.write(f"@{addr:08x}\n")
            f.write(f"{words[addr]}\n")
            current = addr + 1


def main():
    if not INPUT_MEM.exists():
        raise SystemExit(f"Missing {INPUT_MEM}. Run `make build` first.")

    all_words = parse_words(INPUT_MEM)
    text_words = {}
    data_words = {}

    for addr, word in all_words.items():
        if addr >= DATA_WORD_BASE:
            data_words[addr - DATA_WORD_BASE] = word
        else:
            text_words[addr] = word

    write_sparse_mem(TEXT_OUT, text_words)
    write_sparse_mem(DATA_OUT, data_words)
    print(f"Wrote {TEXT_OUT}")
    print(f"Wrote {DATA_OUT}")


if __name__ == "__main__":
    main()
