# main.s - Knight Rider Ana Program
# PicoRV32 / RV32I
# GPIO adresi: 0x10000000
# delay fonksiyonu utils.s'den gelir

.global _start

.text

_start:
    # Stack pointer ayarla: sp = 0x00020000
    lui  sp, 0x20

    # GPIO base adresini t1'e yükle: 0x10000000
    lui  t1, 0x10000

    # Baslangic LED pattern: sadece bit0 = 0x01
    li   t2, 1

knight_loop:

    # === SOLA TARAMA: bit0 -> bit7 ===
    li   t3, 8               # 8 adim

left_scan:
    not  t4, t2              # Aktif-low LED'ler için degeri tersle (0'lar yanar, 1'ler söner)
    sw   t4, 0(t1)           # LED'e yaz (GPIO)
    call delay               # utils.s'deki delay cagir
    slli t2, t2, 1           # biti sola kaydır
    addi t3, t3, -1          # sayaci azalt
    bnez t3, left_scan       # sifir degilse devam

    # === SAGA TARAMA: bit7 -> bit0 ===
    li   t3, 8               # 8 adim

right_scan:
    not  t4, t2              # Aktif-low LED'ler için degeri tersle
    sw   t4, 0(t1)           # LED'e yaz
    call delay               # gecikme
    srli t2, t2, 1           # biti saga kaydır
    addi t3, t3, -1
    bnez t3, right_scan

    # Tekrar baslangica don
    li   t2, 1
    j    knight_loop
