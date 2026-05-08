module top (
    input  wire clk_27mhz,
    input  wire btn_reset,
    output wire [5:0] led
);

    // Active high reset
    wire reset = ~btn_reset;

    // PicoRV32 memory interface
    wire mem_valid;
    wire mem_instr;
    wire mem_ready;
    wire [31:0] mem_addr;
    wire [31:0] mem_wdata;
    wire [3:0]  mem_wstrb;
    wire [31:0] mem_rdata;

    // Address decoding
    // BRAM: 0x00000000 - 0x00001FFF (8KB)
    wire sel_bram = (mem_addr[31:28] == 4'h0);
    // GPIO: 0x80000000
    wire sel_gpio = (mem_addr == 32'h80000000);

    // Slave ready signals
    wire bram_ready;
    wire gpio_ready;

    // Slave read data
    wire [31:0] bram_rdata;
    wire [31:0] gpio_rdata;

    // Mux ready and rdata back to CPU
    assign mem_ready = (sel_bram & bram_ready) | 
                       (sel_gpio & gpio_ready);
                       
    assign mem_rdata = (sel_bram) ? bram_rdata :
                       (sel_gpio) ? gpio_rdata : 32'h0;

    // Instantiate PicoRV32
    picorv32 #(
        .ENABLE_COUNTERS(0),
        .ENABLE_COUNTERS64(0),
        .ENABLE_REGS_16_31(1),
        .ENABLE_REGS_DUALPORT(1),
        .TWO_STAGE_SHIFT(1),
        .BARREL_SHIFTER(0),
        .TWO_CYCLE_COMPARE(0),
        .TWO_CYCLE_ALU(0),
        .COMPRESSED_ISA(0),
        .CATCH_MISALIGN(1),
        .CATCH_ILLINSN(1),
        .ENABLE_PCPI(0),
        .ENABLE_MUL(0),
        .ENABLE_FAST_MUL(0),
        .ENABLE_DIV(0),
        .ENABLE_IRQ(0),
        .ENABLE_IRQ_QREGS(0),
        .ENABLE_IRQ_TIMER(0),
        .PROGADDR_RESET(32'h0000_0000),
        .STACKADDR(32'h0000_2000)
    ) cpu (
        .clk(clk_27mhz),
        .resetn(~reset),
        .mem_valid(mem_valid),
        .mem_instr(mem_instr),
        .mem_ready(mem_ready),
        .mem_addr(mem_addr),
        .mem_wdata(mem_wdata),
        .mem_wstrb(mem_wstrb),
        .mem_rdata(mem_rdata)
    );

    // Instantiate BRAM (8KB = 2048 words)
    bram #(
        .WORDS(2048),
        .INIT_FILE("/home/clar/Sistem Programlama Proje/picorv32_assembler_project_bundle_v2/picorv32_assembler_project/fpga/mem/firmware.hex")
    ) mem_inst (
        .clk(clk_27mhz),
        .valid(mem_valid & sel_bram),
        .wstrb(mem_wstrb),
        .addr(mem_addr[12:2]),
        .wdata(mem_wdata),
        .rdata(bram_rdata),
        .ready(bram_ready)
    );

    // Instantiate GPIO LED
    gpio_led gpio_inst (
        .clk(clk_27mhz),
        .reset(reset),
        .valid(mem_valid & sel_gpio),
        .wstrb(mem_wstrb),
        .wdata(mem_wdata),
        .rdata(gpio_rdata),
        .ready(gpio_ready),
        .led_out(led)
    );


endmodule
