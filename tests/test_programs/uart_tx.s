.text
.global _start
_start:
    li t0, 0x20000000   # UART TX Adresi
    la t1, msg
    
print_loop:
    lb t2, 0(t1)
    beqz t2, end
    sw t2, 0(t0)
    addi t1, t1, 1
    j print_loop
    
end:
    ebreak

.data
msg: .string "Hello PicoRV32!\n"
