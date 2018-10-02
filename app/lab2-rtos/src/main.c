// File: ContextSwitch.c 

#include <stdio.h>
#include "includes.h"
#include <string.h>
#include "stdint.h"
#include "system.h"
#include <time.h>
#include <altera_avalon_performance_counter.h>
#include <sys/alt_timestamp.h>
#include "alt_types.h"

#define DEBUG 0

/* Definition of Task Stacks */
/* Stack grows from HIGH to LOW memory */
#define   TASK_STACKSIZE       2048
OS_STK    task1_stk[TASK_STACKSIZE];
OS_STK    task2_stk[TASK_STACKSIZE];
OS_STK    stat_stk[TASK_STACKSIZE];

/* Definition of Task Priorities */
#define TASK1_PRIORITY      6  // highest priority
#define TASK2_PRIORITY      7
#define TASK_STAT_PRIORITY 12  // lowest priority

OS_EVENT *sem_task0, *sem_task1;

int sharedAddress;
int average = 0;
int average_index = 0;

INT8U err; 

void printStackSize(char* name, INT8U prio) 
{
  INT8U err;
  OS_STK_DATA stk_data;
    
  err = OSTaskStkChk(prio, &stk_data);
  if (err == OS_NO_ERR) {
    if (DEBUG == 1)
      printf("%s (priority %d) - Used: %d; Free: %d\n", 
	     name, prio, stk_data.OSUsed, stk_data.OSFree);
  }
  else
    {
      if (DEBUG == 1)
	printf("Stack Check Error!\n");    
    }
}

/* Prints a message and sleeps for given time interval */
void task1(void* pdata) {
	int send_int = 0;
	while (1) { 
	  	PERF_RESET (PERFORMANCE_COUNTER_BASE);
		send_int++;
		sharedAddress = send_int;
		printf("Sending : %d\n", send_int);
		OSSemPost(sem_task0);
		
		PERF_START_MEASURING (PERFORMANCE_COUNTER_BASE);
		PERF_BEGIN (PERFORMANCE_COUNTER_BASE, 1);
		
		OSSemPend(sem_task1,0,&err);

		PERF_END (PERFORMANCE_COUNTER_BASE, 1);
		PERF_STOP_MEASURING (PERFORMANCE_COUNTER_BASE);
		int ticks = perf_get_section_time(PERFORMANCE_COUNTER_BASE, 1);
		
		int old_average = average;
		average = (average*average_index + ticks) / (average_index + 1);
		average_index++;
		printf("Recieving : %d\n", sharedAddress);
		printf("Context switch time: %d\n", ticks);
		if ((float)ticks > average*1.3)  {
			average = old_average;
			printf("Outlier detected\n" );
		}
		printf("Average Context swtich time: %d\n\n", average);
	}
}

/* Prints a message and sleeps for given time interval */
void task2(void* pdata) {
	while (1) { 	
		OSSemPend(sem_task0,0,&err);
		sharedAddress = sharedAddress*(-1);
		OSSemPost(sem_task1);
	}
}

/* Printing Statistics */
void statisticTask(void* pdata)
{
	while(1) {
		printStackSize("Task1", TASK1_PRIORITY);
		printStackSize("Task2", TASK2_PRIORITY);
		printStackSize("StatisticTask", TASK_STAT_PRIORITY);
	}
}

/* The main function creates two task and starts multi-tasking */
int main(void)
{
	printf("Lab 3 - Two Tasks\n");  

	sem_task0 = OSSemCreate(0);
	sem_task1 = OSSemCreate(0);

	OSTaskCreateExt ( 
		task1,                        // Pointer to task code
		NULL,                         // Pointer to argument passed to task
		&task1_stk[TASK_STACKSIZE-1], // Pointer to top of task stack
		TASK1_PRIORITY,               // Desired Task priority
		TASK1_PRIORITY,               // Task ID
		&task1_stk[0],                // Pointer to bottom of task stack
		TASK_STACKSIZE,               // Stacksize
		NULL,                         // Pointer to user supplied memory (not needed)
		OS_TASK_OPT_STK_CHK |         // Stack Checking enabled 
		OS_TASK_OPT_STK_CLR           // Stack Cleared                                 
	  );
	   
	OSTaskCreateExt ( 
		task2,                        // Pointer to task code
		NULL,                         // Pointer to argument passed to task
		&task2_stk[TASK_STACKSIZE-1], // Pointer to top of task stack
		TASK2_PRIORITY,               // Desired Task priority
		TASK2_PRIORITY,               // Task ID
		&task2_stk[0],                // Pointer to bottom of task stack
		TASK_STACKSIZE,               // Stacksize
		NULL,                         // Pointer to user supplied memory (not needed)
		OS_TASK_OPT_STK_CHK |         // Stack Checking enabled 
		OS_TASK_OPT_STK_CLR           // Stack Cleared                       
	);  

	if (DEBUG == 1) {
		OSTaskCreateExt ( 
		statisticTask,                // Pointer to task code
		NULL,                         // Pointer to argument passed to task
		&stat_stk[TASK_STACKSIZE-1],  // Pointer to top of task stack
		TASK_STAT_PRIORITY,           // Desired Task priority
		TASK_STAT_PRIORITY,           // Task ID
		&stat_stk[0],                 // Pointer to bottom of task stack
		TASK_STACKSIZE,               // Stacksize
		NULL,                         // Pointer to user supplied memory (not needed)
		OS_TASK_OPT_STK_CHK |         // Stack Checking enabled 
		OS_TASK_OPT_STK_CLR           // Stack Cleared                              
		);
	}  

	OSStart();
	return 0;
}
