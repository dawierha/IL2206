#include <stdio.h>
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "alt_types.h"
#include "sys/alt_irq.h"
#include "sys/alt_alarm.h"
#include <math.h>


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
int timeflag = 0;
int alarmflag = 0;

void print(int n)
{
    // If number is smaller than 0, put a - sign and
    // change number to positive
    if (n < 0) {
        putchar('-');
        n = -n;
    }
 
    // If number is 0
    if (n == 0)
        putchar('0');
 
    // Remove the last digit and recur
    if (n/10)
        print(n/10);
 
    // Print the last digit
    putchar(n%10 + '0');
}
 

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

alt_u32 Alarm_Callback(void* context){
	timeflag = 0;
  /* This function will be called once/second */
	if (run)
	    timeloc++;
	alarmflag = 1;

  IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_REDLED18_BASE,timeloc); 
  return alt_ticks_per_second();
}

int nextPrime(int intVal) {
	int i;
	for(i = 2; i <= (int) sqrt(intVal) + 1; i++) {
		if(intVal%i == 0) {
			nextPrime(intVal++);
		}
	}
	return intVal;
}

int main ()
{
	int prime = 2;

     /* set interrupt capability for the Button PIO. */
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(D2_PIO_KEYS4_BASE, 0xf);
     /* Reset the edge capture register. */
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(D2_PIO_KEYS4_BASE, 0x0);
    // Register the ISR for buttons
    alt_irq_register(D2_PIO_KEYS4_IRQ, NULL, Key_InterruptHandler);

    static alt_alarm alarm;  
    /* Register the flashing function for the timer */
    if (alt_alarm_start (&alarm,alt_ticks_per_second(),Alarm_Callback,NULL) < 0)
    {
        printf ("No system clock available\n");
    }

    while (TRUE)
    {

		IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_REDLED18_BASE,timeloc);

		if (buttonflag){
			pollkey();
			buttonflag = 0;
		}

		if (alarmflag){
			puthex(timeloc);	
			puttime (&timeloc);
			alarmflag = 0;
			putchar(0x9);
			print(prime);
			prime = nextPrime(prime);		
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
