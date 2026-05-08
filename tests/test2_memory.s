.text
.org 0x00000000
main:
    lui t0, 0x0
    addi t0, t0, 0x100
    lw t1, 0(t0)
    addi t1, t1, 7
    sw t1, 4(t0)
    jal zero, done
.data
.org 0x00000100
nums:
    .word 10, 20
.text
done:
    ebreak
.end
