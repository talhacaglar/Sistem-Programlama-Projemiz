/* Tang Nano 9K Linker Script */
/* GW1NR-9C FPGA - 8KB BRAM */

MEMORY {
    BRAM : ORIGIN = 0x00000000, LENGTH = 8K
}

SECTIONS {
    .text : { *(.text) } > BRAM
    .data : { *(.data) } > BRAM
}

STACK_TOP = 0x00002000;
