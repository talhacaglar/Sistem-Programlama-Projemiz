.text
.org 0x00000000
start:
    addi a0, zero, 3
    addi a1, zero, 4
    jal ra, add_func
    ecall
add_func:
    add a0, a0, a1
    jalr zero, 0(ra)
.end
