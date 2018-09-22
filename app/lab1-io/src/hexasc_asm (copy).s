        # stub for lab 1, task 2.4
        
        .global hexasc          # makes label "hexasc" globally known

        .text                   # area for instructions

hexasc:		andi r8, 0x0F, r4                 
			ble r8, 9, numLabel
			add r2, r8, 31
			br end
			
numLabel:  	add r2, r8, 48
			br fin
			

fin			ret	
# The assembler will stop here
