#include <stdio.h>
#include "system.h"
#include "altera_avalon_pio_regs.h"

extern void puttime(int* timeloc);
extern void puthex(int time);
extern void tick(int* timeloc);
extern void delay (int millisec);
extern int hexasc(int invalue);

#define TRUE 1

int timeloc = 0x5957; /* startvalue given in hexadecimal/BCD-code */
int swaggerblink = 0;


int main ()
{
    
    while (TRUE)
    {
		delay(1000);
        puttime (&timeloc);
		timeloc++;
		if ((timeloc&0x000F) == 0x000A){ 
			timeloc = (timeloc&0xFFF0)+0x0010;

			if ((timeloc&0x00F0) == 0x0060) 
				timeloc = (timeloc&0xFFF0)+0x00A0;

			if ((timeloc&0x0F00) == 0x0A00) 
				timeloc = (timeloc&0xFF00)+0x0600;

			if ((timeloc&0xF000) == 0x6000) 
				timeloc = 0;
		}
	IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_REDLED18_BASE,swaggerblink);
	swaggerblink = (swaggerblink<<1)+1;

    }
    
    return 0;
}
