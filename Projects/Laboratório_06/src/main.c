#include <stdint.h>
#include <stdbool.h>
#include "system_tm4c1294.h" // CMSIS-Core
#include "driverbuttons.h" // Projects/drivers
#include "driverleds.h"
#include "cmsis_os2.h" 
#define MSGQUEUE_OBJECTS 16
#define QT_LEDS 4
#define QT_LED_PROPERTIES 2 // "selected led" and "duty cycle"
#define PERIOD 10 // in ms
#define STEP_DUTY_CYCLE 10
#define DEBOUNCING_CT 400 // in ms

#define SELECTED_LED 0
#define DUTY_CYCLE 1
#define INITIAL_DUTY_CYCLE 10

volatile int event_usw1 = 0;
volatile int event_usw2 = 0;
int start = 0;

// Mutex Definitions
osMutexId_t phases_mut_id;
typedef enum {BLINK_LED1= 0, BLINK_LED2, BLINK_LED3, BLINK_LED4} state_t;

state_t CurrentState;

const osMutexAttr_t Phases_Mutex_attr = {
  "PhasesMutex",                            // human readable mutex name
  osMutexRecursive | osMutexPrioInherit,    // attr_bits
  NULL,                                     // memory for control block   
  0U                                        // size for control block
};

// Message Queue Definitions
osMessageQueueId_t mid_MsgQueue;

typedef struct {
  char leds[QT_LEDS][QT_LED_PROPERTIES];
} MSGQUEUE_OBJ_t;
 
// Useful Functions
void MutexLEDOn (unsigned char led) {
  osMutexAcquire(phases_mut_id, osWaitForever); // try to acquire mutex
  LEDOn(led);
  osMutexRelease(phases_mut_id);
}
void MutexLEDOff (unsigned char led) {
  osMutexAcquire(phases_mut_id, osWaitForever); // try to acquire mutex
  LEDOff(led);
  osMutexRelease(phases_mut_id);
}

int time_prev = 0;
void PWM(char led, char duty_cycle, int is_intermitent) {
  int time_start, timeout;
  if (is_intermitent) {
    time_start = osKernelGetTickCount();
    timeout = time_start - time_prev;
    if(timeout >= 1000) {
      MutexLEDOff(led);
      osDelay(50);
      time_prev = time_start;
    }
    else {
      MutexLEDOn(led);
      osDelay(PERIOD * duty_cycle / 100);
      MutexLEDOff(led);
      osDelay(PERIOD - PERIOD * duty_cycle / 100);
    }
  }
  else {
    MutexLEDOn(led);
    osDelay(PERIOD * duty_cycle / 100);
    MutexLEDOff(led);
    osDelay(PERIOD - PERIOD * duty_cycle / 100);
  }
}

// Thread Definitions
osThreadId_t tid_ThreadControl;
osThreadId_t tid_ThreadLED1;
osThreadId_t tid_ThreadLED2;
osThreadId_t tid_ThreadLED3;
osThreadId_t tid_ThreadLED4;

void ThreadControl (void *argument) {
  MSGQUEUE_OBJ_t msg;
  
  while (1) {
    if(start == 0) { 
      msg.leds[0][SELECTED_LED] = LED1; // Selected LED
      msg.leds[0][DUTY_CYCLE] = INITIAL_DUTY_CYCLE; // Duty Cycle
      
      msg.leds[1][SELECTED_LED] = LED2; // Selected LED
      msg.leds[1][DUTY_CYCLE] = INITIAL_DUTY_CYCLE; // Duty Cycle
      
      msg.leds[2][SELECTED_LED] = LED3; // Selected LED
      msg.leds[2][DUTY_CYCLE] = INITIAL_DUTY_CYCLE; // Duty Cycle
      
      msg.leds[3][SELECTED_LED] = LED4; // Selected LED
      msg.leds[3][DUTY_CYCLE] = INITIAL_DUTY_CYCLE; // Duty Cycle
      
      start = 1;
    }
    
    if(event_usw2){
      msg.leds[CurrentState][DUTY_CYCLE] = (msg.leds[CurrentState][DUTY_CYCLE] + STEP_DUTY_CYCLE) % 100;
      event_usw2 = 0;
    }
    else if(event_usw1){
      CurrentState = (CurrentState + 1) % (QT_LEDS);
      event_usw1 = 0;
    }
      
    osMessageQueuePut(mid_MsgQueue, &msg, 0U, 0U);
    osThreadYield();
  }
}
 
void ThreadLED (void *argument) {
  MSGQUEUE_OBJ_t msg;
  osStatus_t status;
  volatile int led = (int) argument;
 
  while (1) {
    status = osMessageQueueGet(mid_MsgQueue, &msg, NULL, 0U);   // wait for message
    if(CurrentState == led)
    {
      PWM(msg.leds[led][SELECTED_LED], msg.leds[led][DUTY_CYCLE], 1);
    }
    else
    {
      PWM(msg.leds[led][SELECTED_LED], msg.leds[led][DUTY_CYCLE], 0);
    }
    //osThreadYield();
  }
}

int t_click = 0;

// Interruption Handler Definitions
void GPIOJ_Handler(void)
{
  int t_start_click, timeout;
  ButtonIntClear(USW1 | USW2);
  if(!ButtonRead(USW1)){
    t_start_click = osKernelGetTickCount();
    timeout = t_start_click - t_click;
    if(timeout >= DEBOUNCING_CT) {
      ButtonIntClear(USW1);
      event_usw1 = 1;
    }
  }
  if(!ButtonRead(USW2)) {
    t_start_click = osKernelGetTickCount();
    timeout = t_start_click - t_click;
    if(timeout >= DEBOUNCING_CT) {
      ButtonIntClear(USW2);
      event_usw2 = 1;
    }
  }
  t_click = t_start_click;
  osThreadYield();
}

// Initialization functions
void InitMsgQueue (void) {
  mid_MsgQueue = osMessageQueueNew(MSGQUEUE_OBJECTS, sizeof(MSGQUEUE_OBJ_t), NULL);
  tid_ThreadControl = osThreadNew(ThreadControl, NULL, NULL);
  
  tid_ThreadLED1 = osThreadNew(ThreadLED, (void *) 0, NULL);
  tid_ThreadLED2 = osThreadNew(ThreadLED, (void *) 1, NULL);
  tid_ThreadLED3 = osThreadNew(ThreadLED, (void *) 2, NULL);
  tid_ThreadLED4 = osThreadNew(ThreadLED, (void *) 3, NULL);
}
void initPeripherals (void) {
  LEDInit(LED4 | LED3 | LED2 | LED1);
  ButtonInit(USW1 | USW2);
  ButtonIntEnable(USW1 | USW2);
}

// Main
int main (void) {
  osKernelInitialize();
  
  initPeripherals();
  InitMsgQueue();
  
  CurrentState = BLINK_LED1;

  if (osKernelGetState() == osKernelReady) {
    osKernelStart();
  }

  while(1);
}
