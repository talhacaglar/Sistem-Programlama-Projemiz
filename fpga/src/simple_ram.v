module simple_ram #(
    parameter WORDS = 2048,
    parameter INIT_FILE = "",
    parameter INIT_VALUE = 32'h00000000
) (
    input  wire clk,
    input  wire valid,
    input  wire [3:0] wstrb,
    input  wire [$clog2(WORDS)-1:0] addr,
    input  wire [31:0] wdata,
    output reg  [31:0] rdata,
    output reg  ready
);

    integer i;
    reg [31:0] mem [0:WORDS-1];

    initial begin
        for (i = 0; i < WORDS; i = i + 1)
            mem[i] = INIT_VALUE;

        if (INIT_FILE != "")
            $readmemh(INIT_FILE, mem);
    end

    always @(posedge clk) begin
        ready <= 1'b0;

        if (valid && !ready) begin
            ready <= 1'b1;
            rdata <= mem[addr];

            if (wstrb[0]) mem[addr][7:0]   <= wdata[7:0];
            if (wstrb[1]) mem[addr][15:8]  <= wdata[15:8];
            if (wstrb[2]) mem[addr][23:16] <= wdata[23:16];
            if (wstrb[3]) mem[addr][31:24] <= wdata[31:24];
        end
    end

endmodule
