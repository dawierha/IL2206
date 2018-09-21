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

#define M 64
//#define P_COUNTER_BASE 0x01085000

int matrix[M][M]; 

/* Initialize the matrix */

void initMatrix (int matrix[][M]);
int  sumMatrix  (int matrix[][M], int size);

alt_u32 ticks;
alt_u32 time_1;
alt_u32 time_2;
alt_u32 timer_overhead;



float microseconds(int ticks)
{
  return (float) 1000000 * (float) ticks / (float) alt_timestamp_freq();
}

int main ()
{
  
  int a;
  
  printf("Processor Type: %s\n", NIOS2_CPU_IMPLEMENTATION);

	  PERF_RESET (P_COUNTER_BASE);
      /* Print frequency and period */
      //printf("Timestamp frequency: %3.1f MHz\n", (float)alt_timestamp_freq()/1000000.0);
      //printf("Timestamp period:    %f ms\n\n", 1000.0/(float)alt_timestamp_freq());  

      /* Calculate Timer Overhead */
      // Average of 10 measurements */
      /*int i;
      timer_overhead = 0;
      for (i = 0; i < 10; i++) {      
        start_measurement();
        stop_measurement();
        timer_overhead = timer_overhead + time_2 - time_1;
      }
      timer_overhead = timer_overhead / 10;
        
      printf("Timer overhead in ticks: %d\n", (int) timer_overhead);
      printf("Timer overhead in ms:    %f\n\n", 
	     1000.0 * (float)timer_overhead/(float)alt_timestamp_freq());*/
    
      printf("Measuring sumMatrix...\n");
      initMatrix(matrix);     
//////////////////////////////////////////////////////////////////////////////////
	  
	  int i;
	  int iterations = 50;
 	  float average_time = 0.0;
	  float times[iterations];
	  for(i=0; i<iterations; i++) {
		  initMatrix(matrix);     
		  PERF_START_MEASURING (P_COUNTER_BASE);
      	  a = sumMatrix (matrix, M);
	  	  PERF_STOP_MEASURING (P_COUNTER_BASE);
		  times[i] = microseconds(perf_get_section_time(P_COUNTER_BASE, 1));
		  //printf("Time: %5.2f us\n", times[i]);
		  average_time = average_time +  times[i];
		  PERF_RESET (P_COUNTER_BASE);
	  }
	  average_time = average_time / iterations;
	  float mae = 0.0;
	  for(i=0; i<iterations; i++){
	    mae = mae + abs(average_time - times[i]);
	  }
	  mae = mae / iterations;
	  printf("Average time: %5.2f us\n", average_time);
	  printf("Mena average Error: %5.2f us\n", mae);
//////////////////////////////////////////////////////////////////////////////////    
      //printf("Result: %d\n", a);
	  //unsigned long long time = perf_get_section_time(P_COUNTER_BASE, 1);
      //printf("%d us", time);
      //printf("(%d ticks)\n", (int) (ticks)); 

      printf("Done!\n");
   
  return 0;
}

void initMatrix (int matrix[M][M]){
  int i, j;

  for (i = 0; i < M; i++) {
    for (j = 0; j < M; j++) {
      matrix[i][j] = i+j;
    }
  }
}

int sumMatrix (int matrix[M][M], int size)
{
  int i, j, Sum = 0;
  PERF_BEGIN (P_COUNTER_BASE, 1);
  for (i = 0; i < size; i ++) {
    for (j = 0; j < size; j ++) {
      Sum += matrix[i][j];
    }
  }
  PERF_END (P_COUNTER_BASE, 1);
  return Sum;
}
