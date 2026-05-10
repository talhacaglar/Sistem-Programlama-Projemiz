# knight_rider.s - Knight Rider LED Test Program
# PicoRV32 / RV32I
# GPIO adresi: 0x10000000

.text
.global _start

_start:
    # GPIO base adresini t1'e yukle: 0x10000000
    li   t1, 0x10000000

    # Baslangic LED pattern: sadece bit0 = 0x01
    li   t2, 1

knight_loop:
    # === SOLA TARAMA: bit0 -> bit7 ===
    li   t3, 7               # 7 adim kaydirma

left_scan:
    not  t4, t2              # Aktif-low LED'ler için degeri tersle
    sw   t4, 0(t1)           # LED'e yaz (GPIO)
    
    # Gecikme
    li   t5, 100000
delay_left:
    addi t5, t5, -1
    bnez t5, delay_left

    slli t2, t2, 1           # biti sola kaydir
    addi t3, t3, -1          # sayaci azalt
    bnez t3, left_scan       # sifir degilse devam

    # === SAGA TARAMA: bit7 -> bit0 ===
    li   t3, 7               # 7 adim kaydirma

right_scan:
    not  t4, t2              # Aktif-low LED'ler için degeri tersle
    sw   t4, 0(t1)           # LED'e yaz
    
    # Gecikme
    li   t5, 100000
delay_right:
    addi t5, t5, -1
    bnez t5, delay_right

    srli t2, t2, 1           # biti saga kaydir
    addi t3, t3, -1
    bnez t3, right_scan

    j    knight_loop
