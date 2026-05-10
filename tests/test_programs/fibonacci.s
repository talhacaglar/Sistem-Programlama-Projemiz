.text
.global _start
_start:
    li a0, 0x00010000   # Data base adresi
    li t0, 0            # f(n-2)
    li t1, 1            # f(n-1)
    li t2, 10           # 10 sayi hesapla
    
    sw t0, 0(a0)
    sw t1, 4(a0)
    addi a0, a0, 8
    addi t2, t2, -2
    
loop:
    add t3, t0, t1      # f(n) = f(n-1) + f(n-2)
    sw t3, 0(a0)        # RAM'e yaz
    mv t0, t1
    mv t1, t3
    addi a0, a0, 4
    addi t2, t2, -1
    bnez t2, loop
    ebreak
