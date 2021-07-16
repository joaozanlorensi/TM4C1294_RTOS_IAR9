#include "system_tm4c1294.h" // CMSIS-Core
#include "driverleds.h" // device drivers
#include "cmsis_os2.h" // CMSIS-RTOS

osThreadId_t thread1_id, thread2_id, thread3_id, thread4_id;

typedef struct led_t {
  uint8_t selected_led;
  uint16_t delay;
} led_t;

void my_thread(void *arg){
  led_t* led = (led_t*) arg;
  uint8_t state = 0;
  
  while(1){
    state ^= (led->selected_led);
    LEDWrite(led->selected_led, state);
    osDelay(led->delay);
  } // while
} // my_thread

void main(void){
  LEDInit(LED1 | LED2 | LED3 | LED4);
    
  led_t a = {.selected_led = LED1, .delay = 200};
  led_t b = {.selected_led = LED2, .delay = 300};
  led_t c = {.selected_led = LED3, .delay = 500};
  led_t d = {.selected_led = LED4, .delay = 700};
  
  osKernelInitialize();

  thread1_id = osThreadNew(my_thread, &a, NULL);
  thread2_id = osThreadNew(my_thread, &b, NULL);
  thread3_id = osThreadNew(my_thread, &c, NULL);
  thread4_id = osThreadNew(my_thread, &d, NULL);

  if(osKernelGetState() == osKernelReady)
    osKernelStart();

  while(1);
} // main
