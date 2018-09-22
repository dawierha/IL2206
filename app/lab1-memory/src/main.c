/*
  Functions.c

  Ingo Sander, 2005-10-04
  Johan Wennlund, 2008-09-19
  George Ungureanu, 2018-08-27

*/

#include <stdio.h>
#include <stdlib.h>
#include "system.h"
#include <time.h>
#include <sys/alt_timestamp.h>
#include <altera_avalon_performance_counter.h>
#include "alt_types.h"

float microseconds(int ticks)
{
  return (float) 1000000 * (float) ticks / (float) alt_timestamp_freq();
}


int main ()
{
	int i,j,k;
	int size = 50;
	int memories[3] = {SDRAM_BASE, SRAM_BASE, ONCHIP_MEMORY_BASE};
	PERF_RESET (P_COUNTER_BASE);
	for(i=0; i<3; i++){
		int src = memories[i];
		for(j=0; j<3; j++){
			if (j!=i) {
				int dst = memories[j];
				for(k=1; k<=size; k++){
					PERF_START_MEASURING (P_COUNTER_BASE); 
					PERF_BEGIN (P_COUNTER_BASE, 1);
					memcpy(src, dst, k);
					PERF_END (P_COUNTER_BASE, 1);
					PERF_STOP_MEASURING (P_COUNTER_BASE);
					float time = microseconds(perf_get_section_time(P_COUNTER_BASE, 1));
					printf("size: %d \t src: %d \t dst: %d \t time: %5.2f\n", k, i, j, time);
					PERF_RESET (P_COUNTER_BASE); 
					}
				}
			}
		}
   
  return 0;
}

