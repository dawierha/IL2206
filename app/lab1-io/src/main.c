#include <stdio.h>
#include "system.h"
#include "altera_avalon_pio_regs.h"

extern void puttime(int* timeloc);
extern void puthex(int time);
extern void tick(int* timeloc);
extern void delay (int millisec);
extern int hexasc(int invalue);
extern void puthex(int inval);

#define TRUE 1

int timeloc = 0x5957; /* startvalue given in hexadecimal/BCD-code */

void ticks(int *val){

		int timeloc = *val;

		if ((timeloc&0x000F) == 0x000A){ 
			timeloc = (timeloc&0xFFF0)+0x0010;

			if ((timeloc&0x00F0) == 0x0060) 
				timeloc = (timeloc&0xFFF0)+0x00A0;

			if ((timeloc&0x0F00) == 0x0A00) 
				timeloc = (timeloc&0xFF00)+0x0600;

			if ((timeloc&0xF000) == 0x6000) 
				timeloc = 0;
		}
		
		*val = timeloc;
}


static int b2sLUT[] = {0x40,
                 0x79,
                 0x24,
                 0x30,
                 0x19,
                 0x12,
                 0x02,
                 0x78,
                 0x00,
                 0x18,
                 0x08,
                 0x03,
                 0x27,
                 0x21,
                 0x06,
                 0x0E};

int bcd2seven(int inval)
{
    return b2sLUT[inval];
}

/*
 * puthex - 
 * 
 * Parameter (only one): the time variable.
 */
void puthex( int inval )
{
  /* The return value. */
  int tmp = 0;

  /* Send time to console. */
  tmp = ( bcd2seven( (inval & 0xf000) >> 12 ) << 21) | /* First digit */
        ( bcd2seven( (inval & 0x0f00) >>  8 ) << 14) | /* Second digit */
        ( bcd2seven( (inval & 0x00f0) >>  4 ) <<  7) | /* Third digit */
        ( bcd2seven( (inval & 0x000f)       )      );  /* Last digit */
  //tmp = tmp1 |tmp2|tmp3|tmp4;
  
  IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_HEX_LOW28_BASE,tmp);
}

int main ()
{
    int in;
    while (TRUE)
    {
		delay(1000);
        puttime (&timeloc);
		in = IORD_ALTERA_AVALON_PIO_DATA(D2_PIO_KEYS4_BASE);
		IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_REDLED18_BASE,in);
		puthex(timeloc);
		timeloc++;
		ticks(&timeloc);


    }
    
    return 0;
}
