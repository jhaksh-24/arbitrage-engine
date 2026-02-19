module comparator(
        input wire [63:0] incoming_price,
        input wire [63:0] stored_price,
        input wire stored_valid,
        output wire gt,
        output wire lt,
        output wire eq,
        output wire slot_empty
);

assign gt = (incoming_price > stored_price) && stored_valid;
assign lt = (incoming_price < stored_price) && stored_valid;
assign eq = (incoming_price == stored_price) && stored_valid;
assign slot_empty = !stored_valid;

endmodule
