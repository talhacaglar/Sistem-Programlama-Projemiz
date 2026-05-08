module uart_tx #(
    parameter CLK_HZ = 27_000_000,
    parameter BAUD = 115200
) (
    input  wire clk,
    input  wire reset,
    
    // PicoRV32 memory interface
    input  wire valid,
    input  wire [3:0] addr,  // 0x0/0x4 for Data, 0x8 for Status
    input  wire [3:0] wstrb,
    input  wire [31:0] wdata,
    output reg  [31:0] rdata,
    output reg  ready,
    
    // UART pin
    output reg tx
);

    localparam CLOCKS_PER_BIT = CLK_HZ / BAUD;
    
    // TX State Machine
    localparam STATE_IDLE  = 3'd0;
    localparam STATE_START = 3'd1;
    localparam STATE_DATA  = 3'd2;
    localparam STATE_STOP  = 3'd3;
    
    reg [2:0] state;
    reg [$clog2(CLOCKS_PER_BIT)-1:0] clock_count;
    reg [2:0] bit_index;
    reg [7:0] tx_data;
    
    wire busy = (state != STATE_IDLE);
    
    // Memory mapped interface logic
    always @(posedge clk) begin
        if (reset) begin
            ready <= 1'b0;
            rdata <= 32'b0;
        end else begin
            ready <= 1'b0;
            if (valid && !ready) begin
                ready <= 1'b1;
                if (wstrb != 0) begin
                    // Write
                    if (addr == 4'h4 && !busy) begin
                        // Write to Data Register initiates transmission
                        // State machine will catch this
                    end
                end else begin
                    // Read
                    if (addr == 4'h8) begin
                        // Read Status Register
                        rdata <= {31'b0, busy};
                    end else begin
                        rdata <= 32'b0;
                    end
                end
            end
        end
    end

    // UART TX State Machine
    always @(posedge clk) begin
        if (reset) begin
            state <= STATE_IDLE;
            clock_count <= 0;
            bit_index <= 0;
            tx_data <= 0;
            tx <= 1'b1;
        end else begin
            case (state)
                STATE_IDLE: begin
                    tx <= 1'b1;
                    clock_count <= 0;
                    bit_index <= 0;
                    
                    if (valid && wstrb[0] && addr == 4'h4 && !ready) begin
                        // Start transmission on valid write to Data register
                        tx_data <= wdata[7:0];
                        state <= STATE_START;
                    end
                end
                
                STATE_START: begin
                    tx <= 1'b0;
                    if (clock_count < CLOCKS_PER_BIT - 1) begin
                        clock_count <= clock_count + 1;
                    end else begin
                        clock_count <= 0;
                        state <= STATE_DATA;
                    end
                end
                
                STATE_DATA: begin
                    tx <= tx_data[bit_index];
                    if (clock_count < CLOCKS_PER_BIT - 1) begin
                        clock_count <= clock_count + 1;
                    end else begin
                        clock_count <= 0;
                        if (bit_index < 7) begin
                            bit_index <= bit_index + 1;
                        end else begin
                            state <= STATE_STOP;
                        end
                    end
                end
                
                STATE_STOP: begin
                    tx <= 1'b1;
                    if (clock_count < CLOCKS_PER_BIT - 1) begin
                        clock_count <= clock_count + 1;
                    end else begin
                        state <= STATE_IDLE;
                    end
                end
            endcase
        end
    end

endmodule
