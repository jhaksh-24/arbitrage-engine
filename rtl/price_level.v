module price_level(
        input wire clk,
        input wire reset,
        input wire write_enable,
        input wire [63:0] new_price,
        input wire [63:0] new_quantity,
        input wire clear,
        output wire [63:0] stored_price,
        output wire [63:0] stored_quantity,
        output wire is_valid
);

reg [63:0] price;
reg [63:0] quantity;
reg valid;

always @(posedge clk) begin
        if (reset) begin
                price <= 64'd0;
                quantity <= 64'd0;
                valid <= 1'b0;
        end

        else if (clear) begin
                price <= 64'd0;
                quantity <= 64'd0;
                valid <= 1'b0;
        end

        else if (write_enable) begin
                price <= new_price;
                quantity <= new_quantity;
                valid <= 1'b1;
        end
end

assign stored_price = price;
assign stored_quantity = quantity;
assign is_valid = valid;

endmodule