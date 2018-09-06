#include <stdio.h>
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "alt_types.h"
#include "sys/alt_irq.h"


extern void puttime(int* timeloc);
extern void puthex(int time);
extern void tick(int* timeloc);
extern void delay (int millisec);
extern int hexasc(int invalue);



#define TRUE 1

int run = 0;
int timeloc = 0x0000; /* startvalue given in hexadecimal/BCD-code */
int pushedkey2 = 0;
int buttonflag= 0;

void pollkey(){
	int key = IORD_ALTERA_AVALON_PIO_DATA(D2_PIO_KEYS4_BASE);
		switch (key){
			case 14:
				run = !run;
				break;
			case 13:
				if (!pushedkey2){
					pushedkey2 = 1;
					timeloc++;
				}
				break;
			case 11:
				timeloc = 0x0000;
				break;
			case 7:
				timeloc = 0x5957;
				break;
			default:
				pushedkey2 = 0;
				break;
		}
}


void Key_InterruptHandler(){
    buttonflag = 1;
    /* Write to the edge capture register to reset it. */
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(D2_PIO_KEYS4_BASE, 0);
    /* reset interrupt capability for the Button PIO. */
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(D2_PIO_KEYS4_BASE, 0xf);
}


int main ()
{
	int counter = 0;

     /* set interrupt capability for the Button PIO. */
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(D2_PIO_KEYS4_BASE, 0xf);
     /* Reset the edge capture register. */
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(D2_PIO_KEYS4_BASE, 0x0);
    // Register the ISR for buttons
    alt_irq_register(D2_PIO_KEYS4_IRQ, NULL, Key_InterruptHandler);

    while (TRUE)
    {
		delay(1);
		counter++;
		puthex(timeloc);

		IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_REDLED18_BASE,timeloc);

		if (buttonflag){
			pollkey();
			buttonflag = 0;
		}

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
