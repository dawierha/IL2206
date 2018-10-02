#include <stdio.h>
#include "system.h"
#include "includes.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include "sys/alt_alarm.h"

#define DEBUG 1

#define HW_TIMER_PERIOD 100 /* 100ms */

/* Button Patterns */

#define GAS_PEDAL_FLAG      0x08
#define BRAKE_PEDAL_FLAG    0x04
#define CRUISE_CONTROL_FLAG 0x02
/* Switch Patterns */

#define TOP_GEAR_FLAG       0x00000002
#define ENGINE_FLAG         0x00000001

/* LED Patterns */

#define LED_RED_0 0x00000001 // Engine
#define LED_RED_1 0x00000002 // Top Gear

#define LED_GREEN_0 0x0001 // Cruise Control activated
#define LED_GREEN_2 0x0002 // Cruise Control Button
#define LED_GREEN_4 0x0010 // Brake Pedal
#define LED_GREEN_6 0x0040 // Gas Pedal

/*
 * Definition of Tasks
 */

#define TASK_STACKSIZE 2048

OS_STK StartTask_Stack[TASK_STACKSIZE]; 
OS_STK ControlTask_Stack[TASK_STACKSIZE]; 
OS_STK VehicleTask_Stack[TASK_STACKSIZE];
OS_STK ButtonIO_Stack[TASK_STACKSIZE]; //TODO smaller stach size?
OS_STK SwitchIO_Stack[TASK_STACKSIZE];

// Task Priorities
 
#define STARTTASK_PRIO     5
#define VEHICLETASK_PRIO  10
#define CONTROLTASK_PRIO  12
#define BUTTONIO_PRIO 	  13
#define SWITCHIO_PRIO 	  14

// Task Periods

#define CONTROL_PERIOD  300
#define VEHICLE_PERIOD  300

#define MS_TO_TICKS 50000
#define P_VALUE 5
/*
 * Definition of Kernel Objects 
 */

// Mailboxes
OS_EVENT *Mbox_Throttle;
OS_EVENT *Mbox_Velocity;

// Semaphores

// SW-Timer

/*
 * Types
 */
enum active {on, off};

enum active gas_pedal = off;
enum active brake_pedal = off;
enum active top_gear = off;
enum active engine = off;
enum active cruise_control = off; 

/*
 * Global variables
 */
int delay; // Delay of HW-timer 
INT16U led_green = 0; // Green LEDs
INT32U led_red = 0;   // Red LEDs

OS_TMR *vehicle_tmr;
OS_TMR *control_tmr;
OS_TMR *button_tmr;
OS_TMR *switch_tmr;

OS_EVENT *vehicle_sem;
OS_EVENT *control_sem;
OS_EVENT *button_sem;
OS_EVENT *switch_sem;

INT8U error;


int buttons_pressed(void)
{
  return ~IORD_ALTERA_AVALON_PIO_DATA(D2_PIO_KEYS4_BASE);    
}

int switches_pressed(void)
{
  return IORD_ALTERA_AVALON_PIO_DATA(DE2_PIO_TOGGLES18_BASE);    
}

/*
 * ISR for HW Timer
 */
alt_u32 alarm_handler(void* context)
{
  OSTmrSignal(); /* Signals a 'tick' to the SW timers */
  
  return delay;
}

static int b2sLUT[] = {0x40, //0
		       0x79, //1
		       0x24, //2
		       0x30, //3
		       0x19, //4
		       0x12, //5
		       0x02, //6
		       0x78, //7
		       0x00, //8
		       0x18, //9
		       0x3F, //-
};

/*
 * convert int to seven segment display format
 */
int int2seven(int inval){
  return b2sLUT[inval];
}

/*
 * output current velocity on the seven segement display
 */
void show_velocity_on_sevenseg(INT8S velocity){

 
  int tmp = velocity;
  int out;
  INT8U out_high = 0;
  INT8U out_low = 0;
  INT8U out_sign = 0;

  if(velocity < 0){
    out_sign = int2seven(10);
    tmp *= -1;
  }else{
    out_sign = int2seven(0);
  }

  out_high = int2seven(tmp / 10);
  out_low = int2seven(tmp - (tmp/10) * 10);
  
  out = int2seven(0) << 21 |
    out_sign << 14 |
    out_high << 7  |
    out_low;
  IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_HEX_LOW28_BASE,out);
}

/*
 * shows the target velocity on the seven segment display (HEX5, HEX4)
 * when the cruise control is activated (0 otherwise)
 */
void show_target_velocity(INT8U target_vel)
{
  int tmp = target_vel;
  int out;
  INT8U out_high = 0;
  INT8U out_low = 0;
  INT8U out_sign = 0;



  out_high = int2seven(tmp / 10);
  out_low = int2seven(tmp - (tmp/10) * 10);
  
  out = int2seven(0) << 21 |
	int2seven(0) << 14 |
    out_high << 7  |
    out_low;
  IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_HEX_HIGH28_BASE,out);
}

/*
 * indicates the position of the vehicle on the track with the four leftmost red LEDs
 * LEDR17: [0m, 400m)
 * LEDR16: [400m, 800m)
 * LEDR15: [800m, 1200m)
 * LEDR14: [1200m, 1600m)
 * LEDR13: [1600m, 2000m)
 * LEDR12: [2000m, 2400m]
 */
void show_position(INT16U position)
{
	int redled = IORD_ALTERA_AVALON_PIO_DATA(DE2_PIO_REDLED18_BASE);
	
	if(position >= 20000){
		redled &= 0x0003;
		redled |= 0x000001000;
	}

	else if(position >= 16000){
		redled &= 0x0003;
		redled |= 0x000002000;
	}

	else if(position >= 12000){
		redled &= 0x0003;
		redled |= 0x00004000;
	}

	else if(position >= 8000){
		redled &= 0x0003;
		redled |= 0x00008000;
	}

	else if(position >= 4000){
		redled &= 0x0003;
		redled |= 0x00010000;
	}

	else if(position >= 0){
		redled &= 0x0003;
		redled |= 0x00020000;
	}
    //IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_REDLED18_BASE, redled);
}

/*
 * The function 'adjust_position()' adjusts the position depending on the
 * acceleration and velocity.
 */
INT16U adjust_position(INT16U position, INT16S velocity,
		       INT8S acceleration, INT16U time_interval)
{
  INT16S new_position = position + velocity * time_interval / 1000
    + acceleration / 2  * (time_interval / 1000) * (time_interval / 1000);

  if (new_position > 24000) {
    new_position -= 24000;
  } else if (new_position < 0){
    new_position += 24000;
  }
  
  show_position(new_position);
  return new_position;
}
 
/*
 * The function 'adjust_velocity()' adjusts the velocity depending on the
 * acceleration.
 */
INT16S adjust_velocity(INT16S velocity, INT8S acceleration,  
		       enum active brake_pedal, INT16U time_interval)
{
  INT16S new_velocity;
  INT8U brake_retardation = 200;

  if (brake_pedal == off)
    new_velocity = velocity  + (float) (acceleration * time_interval) / 1000.0;
  else {
    if (brake_retardation * time_interval / 1000 > velocity)
      new_velocity = 0;
    else
      new_velocity = velocity - brake_retardation * time_interval / 1000;
  }
  
  return new_velocity;
}

/*
 * The task 'VehicleTask' updates the current velocity of the vehicle
 */
void VehicleTask(void* pdata)
{ 
  INT8U err;  
  void* msg;
  INT8U* throttle; 
  INT8S acceleration;  /* Value between 40 and -20 (4.0 m/s^2 and -2.0 m/s^2) */
  INT8S retardation;   /* Value between 20 and -10 (2.0 m/s^2 and -1.0 m/s^2) */
  INT16U position = 0; /* Value between 0 and 20000 (0.0 m and 2000.0 m)  */
  INT16S velocity = 0; /* Value between -200 and 700 (-20.0 m/s amd 70.0 m/s) */
  INT16S wind_factor;   /* Value between -10 and 20 (2.0 m/s^2 and -1.0 m/s^2) */

  printf("Vehicle task created!\n");

  while(1)
    {
      err = OSMboxPost(Mbox_Velocity, (void *) &velocity);
      OSSemPend(vehicle_sem, 0, &error);
      //OSTimeDlyHMSM(0,0,0,VEHICLE_PERIOD); 

      /* Non-blocking read of mailbox: 
	 - message in mailbox: update throttle
	 - no message:         use old throttle
      */
      msg = OSMboxPend(Mbox_Throttle, 1, &err); 
      if (err == OS_NO_ERR) 
	throttle = (INT8U*) msg;

      /* Retardation : Factor of Terrain and Wind Resistance */
      if (velocity > 0)
	wind_factor = velocity * velocity / 10000 + 1;
      else 
	wind_factor = (-1) * velocity * velocity / 10000 + 1;
         
      if (position < 4000) 
	retardation = wind_factor; // even ground
      else if (position < 8000)
	retardation = wind_factor + 15; // traveling uphill
      else if (position < 12000)
	retardation = wind_factor + 25; // traveling steep uphill
      else if (position < 16000)
	retardation = wind_factor; // even ground
      else if (position < 20000)
	retardation = wind_factor - 10; //traveling downhill
      else
	retardation = wind_factor - 5 ; // traveling steep downhill
                  
      acceleration = *throttle / 2 - retardation;	  
      position = adjust_position(position, velocity, acceleration, 300); 
      velocity = adjust_velocity(velocity, acceleration, brake_pedal, 300); 
      printf("Position: %dm\n", position / 10);
      printf("Velocity: %4.1fm/s\n", velocity /10.0);
      printf("Throttle: %dV\n", *throttle / 10);
      show_velocity_on_sevenseg((INT8S) (velocity / 10));
    }
} 
 
/*
 * The task 'ControlTask' is the main task of the application. It reacts
 * on sensors and generates responses.
 */

void ControlTask(void* pdata)
{
  INT8U err;
  INT8U throttle = 40; /* Value between 0 and 80, which is interpreted as between 0.0V and 8.0V */
  void* msg;
  INT16S* current_velocity;
  int is_cruise_control = 0;
  int desired_vel = 0;
  int vel_error = 0;
  

  printf("Control Task created!\n");

  while(1)
    {
	  int temp_throttle = (int)throttle;
      OSSemPend(control_sem, 0, &error); 	  
      msg = OSMboxPend(Mbox_Velocity, 0, &err);
      current_velocity = (INT16S*) msg;

	
	  if((top_gear == on) && (cruise_control == on) && (gas_pedal == off) && (brake_pedal == off) 			&& (*current_velocity >= 200) && !is_cruise_control){
		is_cruise_control = 1;
		desired_vel = *current_velocity;
		int greenled = IORD_ALTERA_AVALON_PIO_DATA(DE2_PIO_GREENLED9_BASE);
		IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_GREENLED9_BASE, greenled|0x0001);
		show_target_velocity((INT8U) (desired_vel/10));

	  } else if ((top_gear == off) || (cruise_control == off) || (gas_pedal == on) || (brake_pedal 			== on)	|| (*current_velocity <= 200)) {
		is_cruise_control = 0;
		int greenled = IORD_ALTERA_AVALON_PIO_DATA(DE2_PIO_GREENLED9_BASE);
		IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_GREENLED9_BASE, ((greenled>>1)<<1));
		show_target_velocity((INT8U)0);
	  }
	  
	  if(gas_pedal == on){

	 	temp_throttle += 1;

	  }
		
	  if(is_cruise_control){ //TODO anti windup code
		vel_error = desired_vel - *current_velocity;

		temp_throttle = temp_throttle + P_VALUE*vel_error/10;
		if(temp_throttle < 0){
			temp_throttle = 0;
 		} else if(temp_throttle > 80){
			temp_throttle = 80;
		}

		
		throttle = (INT8U)temp_throttle;
		/*
		printf("vel error %d \n", vel_error/10);
		printf("vel des %d \n", desired_vel);
		printf("current vel %d \n", *current_velocity);
		printf("throttle %d \n", throttle);
		*/
	  }



      err = OSMboxPost(Mbox_Throttle, (void *) &throttle);

      //OSTimeDlyHMSM(0,0,0, CONTROL_PERIOD);
    }
}


/*
 *
 * ButtonIO reads the buttons periodically, and light up coresponding LED
 *
 */
void ButtonIO(void* pdata)
{
  int prev_button = 0;
  while(1){
    OSSemPend(button_sem, 0, &error);
	int greenled = IORD_ALTERA_AVALON_PIO_DATA(DE2_PIO_GREENLED9_BASE);
	int buttons = buttons_pressed();
	int cruise_button = 0; 

	if(buttons&0x00000001 && !(prev_button&0x00000001) ){
		if(cruise_control == on){
		  cruise_control = off;
		  greenled = greenled ^ 0x00000004;
		  }
		else {
		  cruise_control = on;
		  greenled = greenled | 0x00000004;		  
		}
	} else if(buttons&0x00000004 && !(prev_button&0x00000004)){
		if(brake_pedal == on){
		  brake_pedal = off;
		  greenled = greenled | 0x00000010;
		  }
		else {
		  brake_pedal = on;
		  greenled = greenled ^ 0x00000010;		  
		}
	} else if(buttons&0x0000008 && !(prev_button&0x00000008)){
		if(gas_pedal == on){
		  gas_pedal = off;
		  greenled = greenled | 0x00000040;
		  }
		else {
		  gas_pedal = on;
		  greenled = greenled ^ 0x00000040;		  
		}
	}
	
  prev_button = buttons;
  IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_GREENLED9_BASE, greenled);
  }
}

/*
 *
 * SwitchIO reads the Switches periodically.  
 *
 */
void SwitchIO(void* pdata)
{
  while(1){
    OSSemPend(switch_sem, 0, &error);

    int switches = switches_pressed();
	int redled = IORD_ALTERA_AVALON_PIO_DATA(DE2_PIO_REDLED18_BASE);

    if(switches&0x00000001)
      engine = on;
	
	if(!(switches&0x00000001))
      engine = off;
    
	if(switches&0x00000002)
      top_gear = on;
    
	if(!(switches&0x00000002))
      top_gear = off;

	IOWR_ALTERA_AVALON_PIO_DATA(DE2_PIO_REDLED18_BASE, (0x0000003&switches)|redled);
  }
}


/* 
 * The task 'StartTask' creates all other tasks kernel objects and
 * deletes itself afterwards.
 */ 

void StartTask(void* pdata)
{
  INT8U err;
  void* context;

  static alt_alarm alarm;     /* Is needed for timer ISR function */
  
  /* Base resolution for SW timer : HW_TIMER_PERIOD ms */
  delay = alt_ticks_per_second() * HW_TIMER_PERIOD / 1000; 
  printf("delay in ticks %d\n", delay);

  /* 
   * Create Hardware Timer with a period of 'delay' 
   */
  if (alt_alarm_start (&alarm,
		       delay,
		       alarm_handler,
		       context) < 0)
    {
      printf("No system clock available!n");
    }

  /* 
   * Create and start Software Timer 
   */

  /*
   * Creation of Kernel Objects
   */
  
  // Mailboxes
  Mbox_Throttle = OSMboxCreate((void*) 0); /* Empty Mailbox - Throttle */
  Mbox_Velocity = OSMboxCreate((void*) 0); /* Empty Mailbox - Velocity */
   
  /*
   * Create statistics task
   */

  OSStatInit();

  /* 
   * Creating Tasks in the system 
   */


  err = OSTaskCreateExt(
			ControlTask, // Pointer to task code
			NULL,        // Pointer to argument that is
	                // passed to task
			&ControlTask_Stack[TASK_STACKSIZE-1], // Pointer to top
			// of task stack
			CONTROLTASK_PRIO,
			CONTROLTASK_PRIO,
			(void *)&ControlTask_Stack[0],
			TASK_STACKSIZE,
			(void *) 0,
			OS_TASK_OPT_STK_CHK);

  err = OSTaskCreateExt(
			VehicleTask, // Pointer to task code
			NULL,        // Pointer to argument that is
	                // passed to task
			&VehicleTask_Stack[TASK_STACKSIZE-1], // Pointer to top
			// of task stack
			VEHICLETASK_PRIO,
			VEHICLETASK_PRIO,
			(void *)&VehicleTask_Stack[0],
			TASK_STACKSIZE,
			(void *) 0,
			OS_TASK_OPT_STK_CHK);

  err = OSTaskCreateExt(
			ButtonIO, // Pointer to task code
			NULL,        // Pointer to argument that is
	                // passed to task
			&ButtonIO_Stack[TASK_STACKSIZE-1], // Pointer to top
			// of task stack
			BUTTONIO_PRIO,
			BUTTONIO_PRIO,
			(void *)&ButtonIO_Stack[0],
			TASK_STACKSIZE,
			(void *) 0,
			OS_TASK_OPT_STK_CHK);

  err = OSTaskCreateExt(
			SwitchIO, // Pointer to task code
			NULL,        // Pointer to argument that is
	                // passed to task
			&SwitchIO_Stack[TASK_STACKSIZE-1], // Pointer to top
			// of task stack
			SWITCHIO_PRIO,
			SWITCHIO_PRIO,
			(void *)&SwitchIO_Stack[0],
			TASK_STACKSIZE,
			(void *) 0,
			OS_TASK_OPT_STK_CHK);
  
  printf("All Tasks and Kernel Objects generated!\n");

  /* Task deletes itself */

  OSTaskDel(OS_PRIO_SELF);
}



/*
 *
 * Callback from timer, to post semaphore.
 *
 */
void TimerCallback(void *ptmr, void *callback_arg){

  if ((int *)callback_arg == 0){
    OSSemPost(vehicle_sem);
  }

  if ((int *)callback_arg == 1){
    OSSemPost(control_sem);
  }

  if ((int *)callback_arg == 2){
    OSSemPost(button_sem);
  }

  if ((int *)callback_arg == 3){
    OSSemPost(switch_sem);
  }
}




/*
 *
 * The function 'main' creates only a single task 'StartTask' and starts
 * the OS. All other tasks are started from the task 'StartTask'.
 *
 */

int main(void) {

  printf("Lab: Cruise Control\n");



 
  vehicle_tmr = OSTmrCreate(0, 10, OS_TMR_OPT_PERIODIC,
    TimerCallback, (void *)0, "vehicle_tmr", &error);

  control_tmr = OSTmrCreate(0, 10, OS_TMR_OPT_PERIODIC,
    TimerCallback, (void *)1, "control_tmr", &error);

  button_tmr = OSTmrCreate(0, 10, OS_TMR_OPT_PERIODIC,
    TimerCallback, (void *)2, "button_tmr", &error);

  switch_tmr = OSTmrCreate(0, 10, OS_TMR_OPT_PERIODIC,
    TimerCallback, (void *)3, "switch_tmr", &error);

  vehicle_sem = OSSemCreate(0);
  control_sem = OSSemCreate(0);
  button_sem = OSSemCreate(0);
  switch_sem = OSSemCreate(0);
 
  OSTaskCreateExt(
		  StartTask, // Pointer to task code
		  NULL,      // Pointer to argument that is
		  // passed to task
		  (void *)&StartTask_Stack[TASK_STACKSIZE-1], // Pointer to top
		  // of task stack 
		  STARTTASK_PRIO,
		  STARTTASK_PRIO,
		  (void *)&StartTask_Stack[0],
		  TASK_STACKSIZE,
		  (void *) 0,  
		  OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);


  OSTmrStart(vehicle_tmr, &error);         
  OSTmrStart(control_tmr, &error);  
  OSTmrStart(button_tmr, &error);  
  OSTmrStart(switch_tmr, &error);        
  OSStart();
  
  return 0;
}
