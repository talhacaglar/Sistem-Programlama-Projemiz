.text
.global _start
_start:
    la t0, ptr
    ebreak

.data
value: .word 0x12345678
ptr: .word value
