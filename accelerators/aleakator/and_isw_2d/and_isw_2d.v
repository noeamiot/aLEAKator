module top(input a0, input a1, input a2, input b0, input b1, input b2, input z01, input z02, input z12, output c0, output c1, output c2);

    assign a0b1 = a0 & b1;
    assign a1b0 = a1 & b0;

    assign a0b2 = a0 & b2;
    assign a2b0 = a2 & b0;

    assign a1b2 = a1 & b2;
    assign a2b1 = a2 & b1;

    assign a0b0 = a0 & b0;
    assign a1b1 = a1 & b1;
    assign a2b2 = a2 & b2;

    assign a0b1z01 = a0b1 ^ z01;
    assign z1_0 = a0b1z01 ^ a1b0;

    assign a0b2z02 = a0b2 ^ z02;
    assign z2_0 = a0b2z02 ^ a2b0;

    assign a1b2z12 = a1b2 ^ z12;
    assign z2_1 = a1b2z12 ^ a2b1;

    assign c0 = a0b0 ^ z01 ^ z02;
    assign c1 = a1b1 ^ z1_0 ^ z12;
    assign c2 = a2b2 ^ z2_0 ^ z2_1;

endmodule
