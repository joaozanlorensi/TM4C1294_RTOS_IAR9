#include <stdint.h>
#include <stdbool.h>
#include "system_tm4c1294.h" // CMSIS-Core
#include "driverbuttons.h" // Projects/drivers
#include "driverleds.h"
#include "cmsis_os2.h"

#define QT_LEDS 4
#define STEP_DUTY_CYCLE 10
#define PERIOD 10 // Period in ms
#define MSGQUEUE_OBJECTS 4

typedef enum {BLINK_LED1=0, BLINK_LED2, BLINK_LED3, BLINK_LED4} state_t;

state_t CurrentState;

typedef struct {
  unsigned char led;
  uint8_t duty_cycle;
} led_t;

osMessageQueueId_t mid_LED1Queue;
osMessageQueueId_t mid_LED2Queue;
osMessageQueueId_t mid_LED3Queue;
osMessageQueueId_t mid_LED4Queue;

osThreadId_t tid_led;
osThreadId_t tid_control;

led_t thread_LED1_args;
led_t thread_LED2_args;
led_t thread_LED3_args;
led_t thread_LED4_args;

uint8_t event_usw1;
uint8_t event_usw2;

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

void PWM (led_t selected_led) {
  while(1) {
    MutexLEDOn(selected_led.led);
    osDelay(PERIOD * selected_led.duty_cycle/100);
    MutexLEDOff(selected_led.led);
    osDelay(PERIOD - PERIOD * selected_led.duty_cycle/100);
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
    osThreadFlagsWait(0x0001, osFlagsWaitAny, osWaitForever);
    led_t msgLED1, msgLED2, msgLED3, msgLED4;

    if(CurrentState == BLINK_LED1){
      osMessageQueueGet(mid_LED1Queue, &msgLED1, NULL, NULL);
      PWM(msgLED1);
    }
    else if(CurrentState == BLINK_LED2){
      osMessageQueueGet(mid_LED2Queue, &msgLED2, NULL, NULL);
      PWM(msgLED2);
    }
    else if(CurrentState == BLINK_LED3){
      PWM(msgLED3);
      osMessageQueueGet(mid_LED3Queue, &msgLED3, NULL, NULL);
    }
    else if(CurrentState == BLINK_LED4){
      PWM(msgLED4);
      osMessageQueueGet(mid_LED4Queue, &msgLED4, NULL, NULL);
    }
}
}

void setLEDDutyCycle() {
  if(CurrentState == BLINK_LED1) {
    thread_LED1_args = (led_t) {.led = LED1, .duty_cycle = (thread_LED1_args.duty_cycle + STEP_DUTY_CYCLE) % 100};
    thread_LED2_args = (led_t) {.led = LED2, .duty_cycle = 10};
    thread_LED3_args = (led_t) {.led = LED3, .duty_cycle = 10};
    thread_LED4_args = (led_t) {.led = LED4, .duty_cycle = 10};
  }
  if(CurrentState == BLINK_LED1) {
    thread_LED1_args = (led_t) {.led = LED1, .duty_cycle = 10};
    thread_LED2_args = (led_t) {.led = LED2, .duty_cycle = (thread_LED2_args.duty_cycle + STEP_DUTY_CYCLE) % 100};
    thread_LED3_args = (led_t) {.led = LED3, .duty_cycle = 10};
    thread_LED4_args = (led_t) {.led = LED4, .duty_cycle = 10};
  }
  if(CurrentState == BLINK_LED1) {
    thread_LED1_args = (led_t) {.led = LED1, .duty_cycle = 10};
    thread_LED2_args = (led_t) {.led = LED2, .duty_cycle = 10};
    thread_LED3_args = (led_t) {.led = LED3, .duty_cycle = (thread_LED3_args.duty_cycle + STEP_DUTY_CYCLE) % 100};
    thread_LED4_args = (led_t) {.led = LED4, .duty_cycle = 10};
  }
  if(CurrentState == BLINK_LED1) {
    thread_LED1_args = (led_t) {.led = LED1, .duty_cycle = 10};
    thread_LED2_args = (led_t) {.led = LED2, .duty_cycle = 10};
    thread_LED3_args = (led_t) {.led = LED3, .duty_cycle = 10};
    thread_LED4_args = (led_t) {.led = LED4, .duty_cycle = (thread_LED4_args.duty_cycle + STEP_DUTY_CYCLE) % 100};
  }
  
}

void ThreadControl(void *argument) {
  for(;;){
    osThreadFlagsWait(0x0001, osFlagsWaitAny, osWaitForever);
    led_t msg_LED1;
    led_t msg_LED2;
    led_t msg_LED3;
    led_t msg_LED4;
    if(event_usw1 == 1 && event_usw2 == 1) {
      CurrentState = (CurrentState + 1) % QT_LEDS;
      
      event_usw1 = 0;
      event_usw2 = 0;
    }
    else if(event_usw1 == 1) {
      CurrentState = (CurrentState + 1) % QT_LEDS;
      MutexLEDOn(LED1);
      event_usw1 = 0;
    }
    else if(event_usw2 == 1) {
      
      event_usw2 = 0;
    }
    
    osMessageQueuePut(mid_LED1Queue, &msg_LED1, 0U, 0U);
    osMessageQueuePut(mid_LED2Queue, &msg_LED2, 0U, 0U);
    osMessageQueuePut(mid_LED3Queue, &msg_LED3, 0U, 0U);
    osMessageQueuePut(mid_LED4Queue, &msg_LED4, 0U, 0U);
    
    osThreadFlagsSet(tid_led, 0x0001); // Set signal to ThreadControl thread 
    
  }
}

void app_main (void *argument) {
  led_mut_id = osMutexNew(&LED_MutexAttr);
  
  led_t thread_LED1_args = (led_t) {.led = LED1, .duty_cycle = 10};
  led_t thread_LED2_args = (led_t) {.led = LED2, .duty_cycle = 10};
  led_t thread_LED3_args = (led_t) {.led = LED3, .duty_cycle = 10};
  led_t thread_LED4_args = (led_t) {.led = LED4, .duty_cycle = 10};
  
  mid_LED1Queue = osMessageQueueNew(MSGQUEUE_OBJECTS, sizeof(led_t), NULL);
  mid_LED1Queue = osMessageQueueNew(MSGQUEUE_OBJECTS, sizeof(led_t), NULL);
  mid_LED1Queue = osMessageQueueNew(MSGQUEUE_OBJECTS, sizeof(led_t), NULL);
  mid_LED1Queue = osMessageQueueNew(MSGQUEUE_OBJECTS, sizeof(led_t), NULL);
  
  tid_control  = osThreadNew(ThreadControl,  NULL, NULL);
  tid_led  = osThreadNew(ThreadLED,  NULL, NULL);
  
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
  
  osKernelInitialize();                 // Initialize CMSIS-RTOS
  osThreadNew(app_main, NULL, NULL);    // Create application main thread
  if (osKernelGetState() == osKernelReady) {
    osKernelStart();                    // Start thread execution
  }

  while(1);
}
