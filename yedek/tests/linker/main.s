; main.s - Ana program modulu (Tang Nano 9K LED demo)
; Bu dosya _start entry point'i tanimlar ve
; diger modullerden fonksiyonlari cagirir.

.text
.global _start
.extern led_pattern
.extern delay

_start:
    ; Stack pointer kurulumu (8KB BRAM top)
    lui sp, 0x00002         ; SP = 0x00002000

main_loop:
    ; LED yakilma kalibi fonksiyonunu cagir
    jal ra, led_pattern

    ; Sonsuz dongu - tekrar baslat
    jal zero, main_loop

.end
