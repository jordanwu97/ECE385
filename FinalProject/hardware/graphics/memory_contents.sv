//-------------------------------------------------------------------------
//      memory_contents.sv                                               --
//      The memory contents in the test memory.                          --
//                                                                       --
//      Originally contained in test_memory.sv                           --
//      Revised 10-19-2017 by Anand Ramachandran and Po-Han Huang         --
//                        spring 2018 Distribution                       --
//                                                                       --
//      For use with ECE 385 Experment 6                                 --
//      UIUC ECE Department                                              --
//-------------------------------------------------------------------------

module memory_parser;

parameter size = 256;

task memory_contents(output logic[15:0] mem_array[0:size-1]);

	for (integer i = 0; i <= size - 1; i = i + 1)		// Assign the rest of the memory to ffff
	begin
			mem_array[i] = i;
	end

endtask

endmodule
