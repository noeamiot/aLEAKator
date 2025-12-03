// Behaviour level dff
module top (
  input  D, clk,
  output Q, out
);
    assign out = Q;
    always @(posedge clk) begin
        Q <= D;
    end
endmodule

// Gate level dff with reset
//module top (
//  input  D, clk, rst,
//  output Q, Qn
//);
//    always @(posedge clk, posedge rst) begin
//        if (rst) begin
//            Q  <= 0;
//            Qn <= 1;
//        end else begin
//            Q  <= D;
//            Qn <= ~D;
//        end
//    end
//endmodule

// Gate level ff would trigger delta cycles
//module top(input D, input clk, output Q, output q1);
//    wire x,y;
//    assign x = ~(D&clk);
//    assign y = ~(~D&clk);
//    assign q1 = ~(y&Q);
//    assign Q = ~(x&q1);
//endmodule
