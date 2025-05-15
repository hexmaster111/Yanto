module piso_shift_register (
    input   wire        clock,
    input   wire        reset,
    input   wire        load,
    input   wire [3:0]  data_in,
    output  wire        data_out
);
    reg [3:0] shift_reg;
    
    always @(posedge clk or posedge reset) begin
        if (reset) shift_reg <= 4'b0000;
        else if (load) shift_reg <= data_in;
        else shift_reg <= {1'b0, shift_reg[3:1]};
    end

    assign data_out = shift_reg[0];

endmodule

module siso_shift_registers_0(
    input    wire clk,
    input    wire clken,
    input    wire shift_in,
    output   wire shift_out
);

    reg b0, b1, b2, b3;

    always @(posedge clk) begin
        if(clken) begin
            b3 <= b2;
            b2 <= b1;
            b1 <= b0;
            b0 <= shift_in;
        end
    end

    assign shift_out = b3;

endmodule

module sipo_shift_register (
    input   wire        clk,
    input   wire        clken,
    input   wire        reset,
    input   wire        shift_in,
    output  wire [3:0]  out
);
    
    reg [3:0] shift_reg;

    always @(posedge clk or posedge reset) begin
        if(reset)      shift_reg <= 4'b0000;
        else if(clken) shift_reg <= {shift_reg[2:0], shift_in};
    end

    assign out = shift_reg;

endmodule

module bench (
    input   wire        clk,
    input   wire        clken,
    input   wire        reset,
    input   wire        shift_in,
    output  wire [3:0]  out   
);

sipo_shift_register sr (
    .clk(clk),
    .clken(clken),
    .reset(reset),
    .shift_in(shift_in),
    .out(out)
);

endmodule
