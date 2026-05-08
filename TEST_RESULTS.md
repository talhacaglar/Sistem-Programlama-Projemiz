# Test Sonuçları

## Test-1: Sayaç döngüsü (`test1_counter.s`)
- Amaç: `addi`, `blt`, `ecall` doğrulaması
- Durum: Başarılı

Çıktı aralığı:
- `0x00000000 - 0x00000013`

Önemli makine kodları:
- `addi t0, zero, 0` -> `93 02 00 00`
- `addi t1, zero, 5` -> `13 03 50 00`
- `blt  t0, t1, loop` -> `E3 CE 62 FE`
- `ecall` -> `73 00 00 00`

## Test-2: Veri alanı erişimi (`test2_memory.s`)
- Amaç: `.data`, `.org`, `lw`, `sw`, `jal`, `.word` doğrulaması
- Durum: Başarılı

Çıktı aralığı:
- `0x00000000 - 0x0000010B`

Önemli makine kodları:
- `lui  t0, 0x0` -> `B7 02 00 00`
- `addi t0, t0, 0x100` -> `93 82 02 10`
- `lw   t1, 0(t0)` -> `03 A3 02 00`
- `sw   t1, 4(t0)` -> `23 A2 62 00`
- `.word 10, 20` -> `0A 00 00 00 14 00 00 00`
- `ebreak` -> `73 00 10 00`

## Test-3: Alt program çağrısı (`test3_call.s`)
- Amaç: `jal`, `jalr`, R-type toplama ve dönüş akışı doğrulaması
- Durum: Başarılı

Çıktı aralığı:
- `0x00000000 - 0x00000017`

Önemli makine kodları:
- `jal  ra, add_func` -> `EF 00 80 00`
- `add  a0, a0, a1` -> `33 05 B5 00`
- `jalr zero, 0(ra)` -> `67 80 00 00`
