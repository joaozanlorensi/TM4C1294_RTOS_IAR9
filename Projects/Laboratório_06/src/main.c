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

#define SELECTED_LED 0
#define DUTY_CYCLE 1
#define INITIAL_DUTY_CYCLE 10

int event_usw1 = 0;
int event_usw2 = 0;

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
void PWM(char led, char duty_cycle) {
  MutexLEDOn(led);
  osDelay(PERIOD * duty_cycle / 100);
  MutexLEDOff(led);
  osDelay(PERIOD - PERIOD * duty_cycle / 100);
}

// Thread Definitions
osThreadId_t tid_ThreadControl;
osThreadId_t tid_ThreadLED;

void ThreadControl (void *argument) {
  MSGQUEUE_OBJ_t msg;
 
  while (1) {
    if(CurrentState == BLINK_LED1) { 
      msg.leds[0][SELECTED_LED] = LED1; // Selected LED
      msg.leds[0][DUTY_CYCLE] = INITIAL_DUTY_CYCLE; // Duty Cycle
      
      msg.leds[1][SELECTED_LED] = LED2; // Selected LED
      msg.leds[1][DUTY_CYCLE] = INITIAL_DUTY_CYCLE; // Duty Cycle
      
      msg.leds[2][SELECTED_LED] = LED3; // Selected LED
      msg.leds[2][DUTY_CYCLE] = INITIAL_DUTY_CYCLE; // Duty Cycle
      
      msg.leds[3][SELECTED_LED] = LED4; // Selected LED
      msg.leds[3][DUTY_CYCLE] = INITIAL_DUTY_CYCLE; // Duty Cycle
    }
    
    if(event_usw1 && event_usw2){
      msg.leds[CurrentState][DUTY_CYCLE] = (msg.leds[CurrentState][DUTY_CYCLE] + STEP_DUTY_CYCLE) % 100;
      CurrentState = (CurrentState) % (QT_LEDS + 1);
      event_usw1 = 0;
      event_usw2 = 0;
    }
    else if(event_usw2){
      msg.leds[CurrentState][DUTY_CYCLE] = (msg.leds[CurrentState][DUTY_CYCLE] + STEP_DUTY_CYCLE) % 100;
      event_usw2 = 0;
    }
    else if(event_usw1){
      CurrentState = (CurrentState + 1) % (QT_LEDS + 1);
      event_usw1 = 0;
    }
      
    osMessageQueuePut(mid_MsgQueue, &msg, 0U, 0U);
    osThreadYield();
  }
}
 
void ThreadLED (void *argument) {
  MSGQUEUE_OBJ_t msg;
  osStatus_t status;
 
  while (1) {
    status = osMessageQueueGet(mid_MsgQueue, &msg, NULL, 0U);   // wait for message
    if (status == osOK) {
      for (int i = 0; i <= QT_LEDS; i++) {
        PWM(msg.leds[i][SELECTED_LED], msg.leds[i][DUTY_CYCLE]);
      }
      //Tilts the selected LED for a while as so as we can notice that it is the active LED
      PWM(msg.leds[CurrentState][SELECTED_LED], msg.leds[CurrentState][DUTY_CYCLE]);
    }
  }
}

// Interruption Handler Definitions
void GPIOJ_Handler(void)
{
  ButtonIntClear(USW1 | USW2);
  if(ButtonRead(USW1) && ButtonRead(USW2)){
    ButtonIntClear(USW1);
    ButtonIntClear(USW2);
    event_usw1 = 1;
    event_usw2 = 1;
  }
  else if(ButtonRead(USW1)){
    ButtonIntClear(USW1);
    event_usw1 = 1;
  }
  else if(ButtonRead(USW2)) {
    ButtonIntClear(USW2);
    event_usw2 = 1;
  }
}

// Initialization functions
void InitMsgQueue (void) {
  mid_MsgQueue = osMessageQueueNew(MSGQUEUE_OBJECTS, sizeof(MSGQUEUE_OBJ_t), NULL);
  tid_ThreadControl = osThreadNew(ThreadControl, NULL, NULL);
  tid_ThreadLED = osThreadNew(ThreadLED, NULL, NULL);
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
