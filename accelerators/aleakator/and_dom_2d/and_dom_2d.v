module top(input a0, input a1, input a2, input b0, input b1, input b2, input z10, input z20, input z21, input clk, output c0, output c1, output c2);
    wire a0b0, a0b1, a1b0, a1b1, a1b2, a2b1, a0b2, a2b0, a2b2;
    wire a0b1z10, a1b0z10;
    wire a2b1z21, a1b2z21;
    wire a0b2z20, a2b0z20;

    wire ra0b1z10, ra1b0z10;
    wire ra2b1z21, ra1b2z21;
    wire ra0b2z20, ra2b0z20;
    wire prec2, prec1, prec0;

    assign a2b2 = a2 & b2;
    assign a2b0 = a2 & b0;
    assign a0b2 = a0 & b2;

    assign a0b0 = a0 & b0;
    assign a2b1 = a2 & b1;
    assign a1b2 = a1 & b2;

    assign a0b1 = a0 & b1;
    assign a1b0 = a1 & b0;
    assign a1b1 = a1 & b1;

    assign a0b2z20 = a0b2 ^ z20;
    assign a2b0z20 = a2b0 ^ z20;

    assign a2b1z21 = a2b1 ^ z21;
    assign a1b2z21 = a1b2 ^ z21;

    assign a0b1z10 = a0b1 ^ z10;
    assign a1b0z10 = a1b0 ^ z10;

    //assign c0 = a0b0 ^ (a0b1 ^ z);
    //assign c1 = a1b1 ^ (a1b0 ^ z);
    always @(posedge clk) begin
        ra0b2z20 = a0b2z20;
        ra2b0z20 = a2b0z20;

        ra2b1z21 = a2b1z21;
        ra1b2z21 = a1b2z21;

        ra0b1z10 = a0b1z10;
        ra1b0z10 = a1b0z10;
    end

    assign prec2 = a2b2 ^ ra2b0z20;
    assign prec1 = a1b1 ^ ra1b0z10;
    assign prec0 = a0b0 ^ ra0b1z10;

    assign c2 = prec2 ^ ra2b1z21;
    assign c1 = prec1 ^ ra1b2z21;
    assign c0 = prec0 ^ ra0b2z20;
endmodule
