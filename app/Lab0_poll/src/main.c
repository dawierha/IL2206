#include <stdio.h>
#include "system.h"
#include "altera_avalon_pio_regs.h"

extern void puttime(int* timeloc);
extern void puthex(int time);
extern void tick(int* timeloc);
extern void delay (int millisec);
extern int hexasc(int invalue);



#define TRUE 1

int run = 0;
int timeloc = 0x0000; /* startvalue given in hexadecimal/BCD-code */
int pushed = 0;
void pollkey(){
	int key = IORD_ALTERA_AVALON_PIO_DATA(D2_PIO_KEYS4_BASE);
		switch (key){
			case 14:
				run = 1;
				break;
			case 13:
				run = 0;
				break;
			case 11:
				if (!pushed){
					pushed = 1;
					timeloc++;
				}
				break;
			case 7:
				timeloc = 0x0000;
				break;
			default:
				pushed = 0;
				break;
		}
}
int main ()
{
	int counter = 0;
    while (TRUE)
    {
		delay(1);
		counter++;
		pollkey();
		puthex(timeloc);

		IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_REDLED18_BASE,timeloc);

		if (counter == 1000){
			counter = 0;
        	puttime (&timeloc);
			if (run)
				timeloc++;
		}
		
		
		if ((timeloc&0x000F) == 0x000A){ 
			timeloc = (timeloc&0xFFF0)+0x0010;

			if ((timeloc&0x00F0) == 0x0060) 
				timeloc = (timeloc&0xFFF0)+0x00A0;

			if ((timeloc&0x0F00) == 0x0A00) 
				timeloc = (timeloc&0xFF00)+0x0600;

			if ((timeloc&0xF000) == 0x6000) 
				timeloc = 0;
		}



    }
    
    return 0;
}
