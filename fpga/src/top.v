module top (
    input  wire clk_27mhz,
    input  wire btn_reset,
    output wire [5:0] led,
    output wire uart_tx_pin
);

    localparam TEXT_WORDS = 2048;
    localparam DATA_WORDS = 2048;
    localparam TEXT_INIT = "/home/clar/Sistem Programlama Proje/rv32i_c/fpga/mem/firmware_text.mem";
    localparam DATA_INIT = "/home/clar/Sistem Programlama Proje/rv32i_c/fpga/mem/firmware_data.mem";

    wire reset = ~btn_reset;

    wire mem_valid;
    wire mem_instr;
    wire mem_ready;
    wire [31:0] mem_addr;
    wire [31:0] mem_wdata;
    wire [3:0]  mem_wstrb;
    wire [31:0] mem_rdata;

    wire sel_text = (mem_addr[31:16] == 16'h0000) && (mem_addr[12:2] < TEXT_WORDS);
    wire sel_data = (mem_addr[31:16] == 16'h0001);
    wire sel_gpio = (mem_addr == 32'h1000_0000);
    wire sel_uart = (mem_addr[31:4] == 28'h2000_000);

    wire text_ready;
    wire data_ready;
    wire gpio_ready;
    wire uart_ready;
    wire [31:0] text_rdata;
    wire [31:0] data_rdata;
    wire [31:0] gpio_rdata;
    wire [31:0] uart_rdata;
    wire unmapped_ready = mem_valid && !(sel_text || sel_data || sel_gpio || sel_uart);

    assign mem_ready = (sel_text && text_ready) ||
                       (sel_data && data_ready) ||
                       (sel_gpio && gpio_ready) ||
                       (sel_uart && uart_ready) ||
                       unmapped_ready;

    assign mem_rdata = sel_text ? text_rdata :
                       sel_data ? data_rdata :
                       sel_gpio ? gpio_rdata :
                       sel_uart ? uart_rdata :
                       32'h00000000;

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
        .STACKADDR(32'h0002_0000)
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

    simple_ram #(
        .WORDS(TEXT_WORDS),
        .INIT_FILE(TEXT_INIT),
        .INIT_VALUE(32'h00000013)
    ) text_ram (
        .clk(clk_27mhz),
        .valid(mem_valid && sel_text),
        .wstrb(mem_wstrb),
        .addr(mem_addr[12:2]),
        .wdata(mem_wdata),
        .rdata(text_rdata),
        .ready(text_ready)
    );

    simple_ram #(
        .WORDS(DATA_WORDS),
        .INIT_FILE(DATA_INIT),
        .INIT_VALUE(32'h00000000)
    ) data_ram (
        .clk(clk_27mhz),
        .valid(mem_valid && sel_data),
        .wstrb(mem_wstrb),
        .addr(mem_addr[12:2]),
        .wdata(mem_wdata),
        .rdata(data_rdata),
        .ready(data_ready)
    );

    gpio_led gpio_inst (
        .clk(clk_27mhz),
        .reset(reset),
        .valid(mem_valid && sel_gpio),
        .wstrb(mem_wstrb),
        .wdata(mem_wdata),
        .rdata(gpio_rdata),
        .ready(gpio_ready),
        .led_out(led)
    );

    uart_tx uart_inst (
        .clk(clk_27mhz),
        .reset(reset),
        .valid(mem_valid && sel_uart),
        .addr(mem_addr[3:0]),
        .wstrb(mem_wstrb),
        .wdata(mem_wdata),
        .rdata(uart_rdata),
        .ready(uart_ready),
        .tx(uart_tx_pin)
    );

endmodule
