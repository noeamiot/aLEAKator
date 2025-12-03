module top(input a0, input a1, input b0, input b1, input z, input clk, output c0, output c1);
    wire a0b0, a0b1, a1b0, a1b1;
    wire a0b1z, a1b0z;

    assign a0b0 = a0 & b0;
    assign a0b1 = a0 & b1;
    assign a1b0 = a1 & b0;
    assign a1b1 = a1 & b1;

    //assign a1b0 = a1b0 ^ z;
    //assign a0b1 = a0b1 ^ z;
    assign a0b1z = a0b1 ^ z;
    assign a1b0z = a1b0 ^ z;
    
    //assign c0 = a0b0 ^ (a0b1 ^ z);
    //assign c1 = a1b1 ^ (a1b0 ^ z);
    always @(posedge clk) begin
        tmp0 = a1b0z;
        tmp1 = a0b1z;
    end
    assign c0 = a1b1 ^ tmp0;
    assign c1 = a0b0 ^ tmp1;
endmodule
