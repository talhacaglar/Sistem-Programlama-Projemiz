.text
.global _start
_start:
    li t0, 0x10000000   # GPIO LED Adresi
    li t1, 0x01         # LED Durumu
loop:
    sw t1, 0(t0)        # LED'i yak/sondur
    not t1, t1          # Durumu tersine cevir
    li t2, 100000       # Gecikme
delay:
    addi t2, t2, -1
    bnez t2, delay
    j loop
