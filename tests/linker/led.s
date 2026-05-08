; led.s - LED kontrol modulu
; Tang Nano 9K uzerinde 6 adet LED'i sirayla yakar
; GPIO LED register adresi: 0x80000000

.text
.global led_pattern
.extern delay

led_pattern:
    ; Stack frame kaydet
    addi sp, sp, -4
    sw ra, 0(sp)

    ; GPIO base adresini yukle (0x80000000)
    lui t0, 0x80000         ; t0 = 0x80000000
    addi t1, zero, 1        ; t1 = baslangic LED pattern (bit 0)

led_loop:
    ; LED'e yaz
    sw t1, 0(t0)

    ; Gecikme
    jal ra, delay

    ; Sonraki LED'e kaydir
    slli t1, t1, 1

    ; 6 LED mask kontrolu (0x3F = 63)
    andi t2, t1, 63
    bne t2, zero, led_loop

    ; Tum LED'ler yandi, basa don
    addi t1, zero, 1
    jal zero, led_loop

.end
