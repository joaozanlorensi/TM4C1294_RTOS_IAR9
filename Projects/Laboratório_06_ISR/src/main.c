#include <stdint.h>
#include <stdbool.h>
#include "system_tm4c1294.h" // CMSIS-Core
#include "driverbuttons.h" // Projects/drivers
#include "driverleds.h"
#include "cmsis_os2.h"

#define QT_LEDS 4
#define STEP_DUTY_CYCLE 10
#define PERIOD 10 // Period in ms

osThreadId_t tid_LED;
osThreadId_t tid_control;

typedef enum {BLINK_LED1=0, BLINK_LED2, BLINK_LED3, BLINK_LED4} state_t;

typedef struct led_t{
  unsigned char led;
  uint8_t duty_cycle;
}led_t;

led_t leds[QT_LEDS];
int event_usw1;
int event_usw2;

state_t CurrentState;

osMutexId_t led_mut_id;

const osMutexAttr_t LED_MutexAttr = {
  "LED_Mutex",                              // Comprehensive mutex name
  osMutexRecursive | osMutexPrioInherit,    // attr_bits
  NULL,                                     // Memory for control block   
  0U                                        // Size for control block
  };

// Functions
void MutexLEDOn (unsigned char led) {
  osMutexAcquire(led_mut_id, osWaitForever); // Try to acquire mutex
  LEDOn(led);
  osMutexRelease(led_mut_id);
}

void MutexLEDOff (unsigned char led) {
  osMutexAcquire(led_mut_id, osWaitForever); // Try to acquire mutex
  LEDOff(led);
  osMutexRelease(led_mut_id);
}

void PWM () {
  while(1) {
    MutexLEDOn(leds[CurrentState].led);
    osDelay(PERIOD * leds[CurrentState].duty_cycle/100);
    MutexLEDOff(leds[CurrentState].led);
    osDelay(PERIOD - PERIOD * leds[CurrentState].duty_cycle/100);
  }
}

// Interruption Handlers
void GPIOJ_Handler(void)
{
  ButtonIntClear(USW1 | USW2);
  if(ButtonRead(USW1)){
    ButtonIntClear(USW1);
    event_usw1 = 1;
    osThreadFlagsSet(tid_control, 0x0001); // Set signal to thread 'Control'
  }
  else if(ButtonRead(USW2)) {
    ButtonIntClear(USW2);
    event_usw2 = 1;
    osThreadFlagsSet(tid_control, 0x0001); // Set signal to thread 'Control'
  }
}

// Threads
void ThreadLED (void *argument) {
  for (;;) {
    osThreadFlagsWait(0x0001, osFlagsWaitAny ,osWaitForever);    /* wait for an event flag 0x0001 */
    PWM();
  }
}

void ThreadControl(void *argument) {
  for(;;){
    osThreadFlagsWait(0x0001, osFlagsWaitAny, osWaitForever);
    if(event_usw1 == 1 && event_usw2 == 1) {
      CurrentState = (CurrentState + 1) % QT_LEDS;
      leds[CurrentState].duty_cycle = (leds[CurrentState].duty_cycle + STEP_DUTY_CYCLE) % 100;
      event_usw1 = 0;
      event_usw2 = 0;
    }
    if(event_usw1 == 1) {
      CurrentState = (CurrentState + 1) % QT_LEDS;
      event_usw1 = 0;
    }
    else if(event_usw2 == 1) {
      leds[CurrentState].duty_cycle = (int) (leds[CurrentState].duty_cycle + STEP_DUTY_CYCLE) % 100;
      event_usw2 = 0;
    }
    MutexLEDOff(leds[(CurrentState + 1) % QT_LEDS].led);
    MutexLEDOff(leds[(CurrentState + 2) % QT_LEDS].led);
    MutexLEDOff(leds[(CurrentState + 3) % QT_LEDS].led);
    osThreadFlagsSet(tid_LED, 0x0001);
  }
}

void app_main (void *argument) {
  tid_LED = osThreadNew(ThreadLED, NULL, NULL);
  tid_control  = osThreadNew(ThreadControl,  NULL, NULL);
  
  led_mut_id = osMutexNew(&LED_MutexAttr);
  
  CurrentState = 0;
  osThreadFlagsSet(tid_control, 0x0001); // Set signal to ThreadControl thread 

  osDelay(osWaitForever);
  while(1);
}

int main (void) {
  // System Initialization
  SystemInit();
  ButtonInit(USW1);
  ButtonInit(USW2);
  ButtonIntEnable(USW1);
  ButtonIntEnable(USW2);
  LEDInit(LED4 | LED3 | LED2 | LED1);
  
  event_usw1 = 0;
  event_usw2 = 0;
  
  leds[0] = (led_t) {.led = LED1, .duty_cycle = 10};
  leds[1] = (led_t) {.led = LED2, .duty_cycle = 10};
  leds[2] = (led_t) {.led = LED3, .duty_cycle = 10};
  leds[3] = (led_t) {.led = LED4, .duty_cycle = 10};

  osKernelInitialize();                 // Initialize CMSIS-RTOS
  osThreadNew(app_main, NULL, NULL);    // Create application main thread
  if (osKernelGetState() == osKernelReady) {
    osKernelStart();                    // Start thread execution
  }

  while(1);
}
