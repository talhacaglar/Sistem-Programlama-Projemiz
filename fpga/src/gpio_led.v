module gpio_led (
    input  wire clk,
    input  wire reset,
    input  wire valid,
    input  wire [3:0] wstrb,
    input  wire [31:0] wdata,
    output reg  [31:0] rdata,
    output reg  ready,
    output wire [5:0] led_out
);

    reg [5:0] led_reg;

    // Tang Nano 9K LEDs are active-low. The firmware already writes active-low
    // values, so drive the pins directly.
    assign led_out = led_reg;

    always @(posedge clk) begin
        if (reset) begin
            led_reg <= 6'b111111;
            ready <= 1'b0;
            rdata <= 32'b0;
        end else begin
            ready <= 1'b0;

            if (valid && !ready) begin
                ready <= 1'b1;
                if (wstrb[0])
                    led_reg <= wdata[5:0];
                rdata <= {26'b0, led_reg};
            end
        end
    end

endmodule
