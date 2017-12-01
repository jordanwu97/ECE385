module testbench();
timeunit 10ns; // Half clock cycle at 50 MHz
// This is the amount of time represented by #1
timeprecision 1ns;
// These signals are internal because the processor will be
// instantiated as a submodule in testbench.
logic        Clk, RESET_H;
logic [7:0]  VGA_R, VGA_G, VGA_B;
logic VGA_CLK, VGA_SYNC_N, VGA_BLANK_N, VGA_VS, VGA_HS;
// SRAM passthrough
logic SRAM_CE_N, SRAM_UB_N, SRAM_LB_N, SRAM_OE_N, SRAM_WE_N;
logic [19:0] SRAM_ADDR;
wire [15:0] SRAM_DQ; //tristate buffers need to be of type wire

// Instantiating the cpu
graphics_module gw(.CLK_50(Clk), .*);

// Toggle the clock
// #1 means wait for a delay of 1 timeunit
always begin : CLOCK_GENERATION
#1 Clk = ~Clk;
end
initial begin: CLOCK_INITIALIZATION
 Clk = 0;
end
// Testing begins here
// The initial block is not synthesizable
// Everything happens sequentially inside an initial block
// as in a software program
initial begin: TEST_VECTORS
RESET_H = 1;
#2 RESET_H = 0; // Toggle Rest

end
endmodule