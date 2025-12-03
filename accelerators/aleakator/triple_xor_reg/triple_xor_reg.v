module top(input clk, input a, input b, input c, input d, output s);
    wire tmp1;
    wire tmp2;
    wire tmp3;
    assign tmp1 = a ^ b;
    assign tmp3 = tmp2 ^ c;
    assign s = tmp3 ^ d;

    always @(posedge clk) begin
        tmp2 <= tmp1;
    end;
endmodule
