module top(input a0, input a1, input b0, input b1, input z, output c0, output c1);
    wire a0b0, a0b1, a1b0, a1b1;
    wire a0b1z, a0b1za1b0;

    assign a0b0 = a0 & b0;
    assign a0b1 = a0 & b1;
    assign a1b0 = a1 & b0;
    assign a1b1 = a1 & b1;

    assign a0b1z = a0b1 ^ z;
    assign a0b1za1b0 = a0b1z ^ a1b0;
    
    assign c0 = a0b0 ^ z;
    assign c1 = a0b1za1b0 ^ a1b1;

endmodule
