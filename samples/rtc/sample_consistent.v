
module consistent(input clk, 
                     input i, output o);
  reg state;
  reg v;
  assign o = v == 0;
  initial
  begin
    state = 0;
    v = 0;
  end
  always @(posedge clk)
  begin
    if (state == 0 && i == 1) begin
      state <= 1;
      v <= v;
    end
    else begin
        if (i == 1) begin
          v <= 1;
        end
        state <= 0;
    end
  end
//   always @* begin
//     assert(v == 0);
//   end
endmodule
