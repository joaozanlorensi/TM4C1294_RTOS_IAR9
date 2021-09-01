// Libraries
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "utils/uartstdio.h"
#include "driverleds.h"
#include "driverbuttons.h" 
#include "UART_Config.h"
#include "inc/hw_uart.h"
#include "system_tm4c1294.h" 
#include "cmsis_os2.h" 
#include "inc/hw_memmap.h"

// Constants
#define MSGQUEUE_OBJECTS 16
#define DEBOUNCING_CT 400 // in ms
#define UART_TX_BUFFER_SIZE 1024
#define BUFFER_READ_SIZE 12
#define BAUD_RATE 115200

// Global Variables
volatile int event_reset = 0;
volatile int event_usw1 = 0;
volatile int event_usw2 = 0;
int turn = 0;
int go_straight_ind = 0;
char uart_input[BUFFER_READ_SIZE];

typedef enum {START= 0, GO_STRAIGHT, TURN_LEFT, TURN_RIGHT, STOP, HARD_STOP} state_t;
state_t CurrentState;
state_t PrevState;

// Mutex Definitions

osMutexId_t UART_mutex_id;
const osMutexAttr_t UART_mutex_attr = {
  "UART Mutex",                             // human readable mutex name
  osMutexRecursive | osMutexPrioInherit,    // attr_bits
  NULL,                                     // memory for control block   
  0U                                        // size for control block
};

osMutexId_t LED_mutex_id;
const osMutexAttr_t LED_mutex_attr = {
  "LEDs Mutex",                             // human readable mutex name
  osMutexRecursive | osMutexPrioInherit,    // attr_bits
  NULL,                                     // memory for control block   
  0U                                        // size for control block
};

// Message Queue Definitions
osMessageQueueId_t mid_MsgQueue;

typedef struct {
  int turn_side;  
} MSGQUEUE_OBJ_t;
 
// Useful Functions
void MutexLEDOn (unsigned char led) {
  osMutexAcquire(LED_mutex_id, osWaitForever); // try to acquire mutex
  LEDOn(led);
  osMutexRelease(LED_mutex_id);
}
void MutexLEDOff (unsigned char led) {
  osMutexAcquire(LED_mutex_id, osWaitForever); // try to acquire mutex
  LEDOff(led);
  osMutexRelease(LED_mutex_id);
}
void MutexUARTPrintf (char* message) {
  osMutexAcquire(UART_mutex_id, osWaitForever); // try to acquire mutex
  UARTprintf(message);
  UARTFlushTx(false);
  osMutexRelease(UART_mutex_id);
}
void MutexUARTgets () {
  UARTgets(uart_input, BUFFER_READ_SIZE);
}

// Thread Definitions
osThreadId_t tid_ThreadManager;
osThreadId_t tid_ThreadController;

void ThreadManager (void *argument) {
  MSGQUEUE_OBJ_t msg;
  while (1) {    
    PrevState = CurrentState;
    //UARTprintf("A10;");
    MutexUARTgets();
    
    if(CurrentState == START) {
      msg.turn_side = 0;
    }
    
    if (uart_input[0] == 'i' || go_straight_ind) { // Reto
      CurrentState = GO_STRAIGHT;
    }
    else if (uart_input[0] == 'x') { // Parada
      CurrentState = HARD_STOP;
    }
    else if (uart_input[0] == 'L') { // Curva acionada pelo alerta do sensor Lrf
      CurrentState = HARD_STOP;
      turn = 1;
      if(msg.turn_side == 0) {
        msg.turn_side = 1;
      }
      else {
        msg.turn_side = 0;
      }
    }
     
    osMessageQueuePut(mid_MsgQueue, &msg, 0U, 0U);
    osThreadYield();
  }
}
 
void ThreadController (void *argument) {
  MSGQUEUE_OBJ_t msg;
  volatile int led = (int) argument;
  
  while (1) {
    osMessageQueueGet(mid_MsgQueue, &msg, NULL, 0U);   // wait for message
    
    if(CurrentState == GO_STRAIGHT && PrevState != CurrentState || go_straight_ind)
    {
      MutexUARTPrintf("A10;");
      go_straight_ind = 0;
    }
    else if (CurrentState == HARD_STOP && PrevState != CurrentState || turn)
    {
      MutexUARTPrintf("E;");
      if(turn) {
        if(msg.turn_side == 1) {
          CurrentState = TURN_LEFT;
        }
        else {
          CurrentState = TURN_RIGHT;
        }
        turn = 0;
      }
    }
    if(CurrentState == TURN_LEFT && PrevState != CurrentState) {
      MutexUARTPrintf("V45;");
      go_straight_ind = 1;
      CurrentState = GO_STRAIGHT;
    }
    else if(CurrentState == TURN_RIGHT && PrevState != CurrentState) {
      MutexUARTPrintf("V-45;");
      go_straight_ind = 1;
      CurrentState = GO_STRAIGHT;
    }
    osThreadYield();
  }
}

// Initialization functions
void InitMsgQueueAndThreads (void) {
  mid_MsgQueue = osMessageQueueNew(MSGQUEUE_OBJECTS, sizeof(MSGQUEUE_OBJ_t), NULL);
  
  tid_ThreadManager = osThreadNew(ThreadManager, NULL, NULL);
  tid_ThreadController = osThreadNew(ThreadController, NULL, NULL);
  
  UART_mutex_id = osMutexNew(&UART_mutex_attr);
  LED_mutex_id = osMutexNew(&LED_mutex_attr);
}
void UARTInit(void){
  // Enable UART0
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));

  // Initialize the UART for console I/O.
  UARTStdioConfig(0, BAUD_RATE, SystemCoreClock);

  // Enable the GPIO Peripheral used by the UART.
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));

  // Configure GPIO Pins for UART mode.
  GPIOPinConfigure(GPIO_PA0_U0RX);
  GPIOPinConfigure(GPIO_PA1_U0TX);
  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
}
void initPeripherals (void) {
  LEDInit(LED4 | LED3 | LED2 | LED1);
  UARTInit();
}

// Main
int main (void) {
  osKernelInitialize();
  
  initPeripherals();
  InitMsgQueueAndThreads();
  
  CurrentState = START;
  
  if (osKernelGetState() == osKernelReady) {
    osKernelStart();
  }
  while(1);
}