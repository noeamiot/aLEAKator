module top(
    input a0,
    input a1,
    input a2,
    input a3,
    input a4,
    input a5,
    input b0,
    input b1,
    input b2,
    input b3,
    input b4,
    input b5,
    input z10,
    input z20,
    input z21,
    input z30,
    input z31,
    input z32,
    input z40,
    input z41,
    input z42,
    input z43,
    input z50,
    input z51,
    input z52,
    input z53,
    input z54,
    output c0,
    output c1,
    output c2,
    output c3,
    output c4,
    output c5,
input clk);
assign a0b0 = (a0 & b0);
assign a0b1 = (a0 & b1);
assign a0b2 = (a0 & b2);
assign a0b3 = (a0 & b3);
assign a0b4 = (a0 & b4);
assign a0b5 = (a0 & b5);
assign a1b0 = (a1 & b0);
assign a1b1 = (a1 & b1);
assign a1b2 = (a1 & b2);
assign a1b3 = (a1 & b3);
assign a1b4 = (a1 & b4);
assign a1b5 = (a1 & b5);
assign a2b0 = (a2 & b0);
assign a2b1 = (a2 & b1);
assign a2b2 = (a2 & b2);
assign a2b3 = (a2 & b3);
assign a2b4 = (a2 & b4);
assign a2b5 = (a2 & b5);
assign a3b0 = (a3 & b0);
assign a3b1 = (a3 & b1);
assign a3b2 = (a3 & b2);
assign a3b3 = (a3 & b3);
assign a3b4 = (a3 & b4);
assign a3b5 = (a3 & b5);
assign a4b0 = (a4 & b0);
assign a4b1 = (a4 & b1);
assign a4b2 = (a4 & b2);
assign a4b3 = (a4 & b3);
assign a4b4 = (a4 & b4);
assign a4b5 = (a4 & b5);
assign a5b0 = (a5 & b0);
assign a5b1 = (a5 & b1);
assign a5b2 = (a5 & b2);
assign a5b3 = (a5 & b3);
assign a5b4 = (a5 & b4);
assign a5b5 = (a5 & b5);

assign a1zb0 = (a1b0 ^ z10);
assign a0zb1 = (a0b1 ^ z10);
assign a2zb0 = (a2b0 ^ z20);
assign a0zb2 = (a0b2 ^ z20);
assign a2zb1 = (a2b1 ^ z21);
assign a1zb2 = (a1b2 ^ z21);
assign a3zb0 = (a3b0 ^ z30);
assign a0zb3 = (a0b3 ^ z30);
assign a3zb1 = (a3b1 ^ z31);
assign a1zb3 = (a1b3 ^ z31);
assign a3zb2 = (a3b2 ^ z32);
assign a2zb3 = (a2b3 ^ z32);
assign a4zb0 = (a4b0 ^ z40);
assign a0zb4 = (a0b4 ^ z40);
assign a4zb1 = (a4b1 ^ z41);
assign a1zb4 = (a1b4 ^ z41);
assign a4zb2 = (a4b2 ^ z42);
assign a2zb4 = (a2b4 ^ z42);
assign a4zb3 = (a4b3 ^ z43);
assign a3zb4 = (a3b4 ^ z43);
assign a5zb0 = (a5b0 ^ z50);
assign a0zb5 = (a0b5 ^ z50);
assign a5zb1 = (a5b1 ^ z51);
assign a1zb5 = (a1b5 ^ z51);
assign a5zb2 = (a5b2 ^ z52);
assign a2zb5 = (a2b5 ^ z52);
assign a5zb3 = (a5b3 ^ z53);
assign a3zb5 = (a3b5 ^ z53);
assign a5zb4 = (a5b4 ^ z54);
assign a4zb5 = (a4b5 ^ z54);
always @(posedge clk) begin
  ra1b0 = (a1zb0);
  ra0b1 = (a0zb1);
  ra2b0 = (a2zb0);
  ra0b2 = (a0zb2);
  ra2b1 = (a2zb1);
  ra1b2 = (a1zb2);
  ra3b0 = (a3zb0);
  ra0b3 = (a0zb3);
  ra3b1 = (a3zb1);
  ra1b3 = (a1zb3);
  ra3b2 = (a3zb2);
  ra2b3 = (a2zb3);
  ra4b0 = (a4zb0);
  ra0b4 = (a0zb4);
  ra4b1 = (a4zb1);
  ra1b4 = (a1zb4);
  ra4b2 = (a4zb2);
  ra2b4 = (a2zb4);
  ra4b3 = (a4zb3);
  ra3b4 = (a3zb4);
  ra5b0 = (a5zb0);
  ra0b5 = (a0zb5);
  ra5b1 = (a5zb1);
  ra1b5 = (a1zb5);
  ra5b2 = (a5zb2);
  ra2b5 = (a2zb5);
  ra5b3 = (a5zb3);
  ra3b5 = (a3zb5);
  ra5b4 = (a5zb4);
  ra4b5 = (a4zb5);
end
assign c0 = a0b0 ^ ra0b1 ^ ra0b2 ^ ra0b3 ^ ra0b4 ^ ra0b5;
assign c1 = a1b1 ^ ra1b0 ^ ra1b2 ^ ra1b3 ^ ra1b4 ^ ra1b5;
assign c2 = a2b2 ^ ra2b0 ^ ra2b1 ^ ra2b3 ^ ra2b4 ^ ra2b5;
assign c3 = a3b3 ^ ra3b0 ^ ra3b1 ^ ra3b2 ^ ra3b4 ^ ra3b5;
assign c4 = a4b4 ^ ra4b0 ^ ra4b1 ^ ra4b2 ^ ra4b3 ^ ra4b5;
assign c5 = a5b5 ^ ra5b0 ^ ra5b1 ^ ra5b2 ^ ra5b3 ^ ra5b4;
endmodule
