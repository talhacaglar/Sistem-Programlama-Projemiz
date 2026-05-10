# utils.s - Yardimci Fonksiyonlar
# PicoRV32 / RV32I
# Icerik: delay fonksiyonu + .data bolumu

.global delay
.global delay_val

.data

delay_val:
    .word 200000             # Gecikme sayaci (FPGA frekansina gore ayarla)

counter_buf:
    .word 0                  # Genel amacli sayac tamponu

.text

# ---------------------------------------------------
# delay: Yaklasik 200000 cevrim bekler
# Bozulan register: t4, t5
# ---------------------------------------------------
delay:
    la   t5, delay_val       # delay_val adresini t5'e yukle (LA relocation)
    lw   t4, 0(t5)           # delay_val degerini oku

delay_loop:
    addi t4, t4, -1          # sayaci azalt
    bnez t4, delay_loop      # sifir degilse donguye devam
    ret                      # cagirana don (jalr x0, ra, 0)
