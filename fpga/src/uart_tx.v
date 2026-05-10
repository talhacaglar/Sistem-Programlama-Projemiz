module uart_tx #(
    parameter CLK_HZ = 27_000_000,
    parameter BAUD = 115200
) (
    input  wire clk,
    input  wire reset,
    input  wire valid,
    input  wire [3:0] addr,
    input  wire [3:0] wstrb,
    input  wire [31:0] wdata,
    output reg  [31:0] rdata,
    output reg  ready,
    output reg  tx
);

    localparam CLOCKS_PER_BIT = CLK_HZ / BAUD;
    localparam STATE_IDLE  = 2'd0;
    localparam STATE_START = 2'd1;
    localparam STATE_DATA  = 2'd2;
    localparam STATE_STOP  = 2'd3;

    reg [1:0] state;
    reg [$clog2(CLOCKS_PER_BIT)-1:0] clock_count;
    reg [2:0] bit_index;
    reg [7:0] tx_data;

    wire busy = (state != STATE_IDLE);
    wire write_data = valid && wstrb[0] && addr == 4'h0 && !busy;

    always @(posedge clk) begin
        if (reset) begin
            ready <= 1'b0;
            rdata <= 32'b0;
        end else begin
            ready <= 1'b0;
            if (valid && !ready) begin
                ready <= 1'b1;
                if (addr == 4'h4)
                    rdata <= {31'b0, busy};
                else
                    rdata <= 32'b0;
            end
        end
    end

    always @(posedge clk) begin
        if (reset) begin
            state <= STATE_IDLE;
            clock_count <= 0;
            bit_index <= 0;
            tx_data <= 8'h00;
            tx <= 1'b1;
        end else begin
            case (state)
                STATE_IDLE: begin
                    tx <= 1'b1;
                    clock_count <= 0;
                    bit_index <= 0;

                    if (write_data) begin
                        tx_data <= wdata[7:0];
                        state <= STATE_START;
                    end
                end

                STATE_START: begin
                    tx <= 1'b0;
                    if (clock_count < CLOCKS_PER_BIT - 1) begin
                        clock_count <= clock_count + 1'b1;
                    end else begin
                        clock_count <= 0;
                        state <= STATE_DATA;
                    end
                end

                STATE_DATA: begin
                    tx <= tx_data[bit_index];
                    if (clock_count < CLOCKS_PER_BIT - 1) begin
                        clock_count <= clock_count + 1'b1;
                    end else begin
                        clock_count <= 0;
                        if (bit_index < 7)
                            bit_index <= bit_index + 1'b1;
                        else
                            state <= STATE_STOP;
                    end
                end

                STATE_STOP: begin
                    tx <= 1'b1;
                    if (clock_count < CLOCKS_PER_BIT - 1) begin
                        clock_count <= clock_count + 1'b1;
                    end else begin
                        state <= STATE_IDLE;
                    end
                end
            endcase
        end
    end

endmodule
