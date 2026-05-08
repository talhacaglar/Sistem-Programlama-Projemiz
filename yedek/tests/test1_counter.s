.text
.org 0x00000000
start:
    addi t0, zero, 0
    addi t1, zero, 5
loop:
    addi t0, t0, 1
    blt t0, t1, loop
    ecall
.end
