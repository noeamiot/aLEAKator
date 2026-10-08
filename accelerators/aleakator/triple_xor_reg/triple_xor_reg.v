module top(input clk, input a, input b, input c, input d, output s);
    wire tmp1;
    wire tmp3;
    reg tmp2;
    assign tmp1 = a ^ b;
    assign tmp3 = tmp2 ^ c;
    assign s = tmp3 ^ d;

    always @(posedge clk) begin
        tmp2 <= tmp1;
    end;
endmodule
