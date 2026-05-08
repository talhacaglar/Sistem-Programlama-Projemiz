; utils.s - Yardimci fonksiyonlar
; delay fonksiyonu saglar

.text
.global delay

; delay - yaklasik gecikme fonksiyonu
; 27MHz clock icin ~250ms gecikme
delay:
    lui t3, 0x000A0         ; t3 = ~655360 dongu
delay_loop:
    addi t3, t3, -1
    bne t3, zero, delay_loop
    jalr zero, 0(ra)        ; return

.end
