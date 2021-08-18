// Adaptado do Projeto prodcons

#include <stdint.h>
#include <stdbool.h>
#include "system_tm4c1294.h" // CMSIS-Core
#include "driverleds.h" // device drivers
#include "cmsis_os2.h" // CMSIS-RTOS
// includes da biblioteca driverlib
#include "driverbuttons.h" // Projects/drivers

#define BUFFER_SIZE 8
#define NO_WAIT 0

osThreadId_t consumidor_id;
osSemaphoreId_t vazio_id, cheio_id;
uint8_t buffer[BUFFER_SIZE];
uint8_t count = 0;
uint8_t index_p = 0;
int t_prev_interrupt = 0;

// GPIOJ_Handler atua como produtor
void GPIOJ_Handler(void)
{
  ButtonIntClear(USW1);
  
  osSemaphoreAcquire(vazio_id, NO_WAIT);
  buffer[index_p] = count; // coloca no buffer
  osSemaphoreRelease(cheio_id);
  
  index_p++; // incrementa índice de colocação no buffer
  if(index_p >= BUFFER_SIZE)
    index_p = 0;
  
  count++;
  count &= 0x0F; // produz nova informação
  
  osThreadYield();
}

void consumidor(void *arg){
  uint8_t index_c = 0, state;
  
  while(1){
    osSemaphoreAcquire(cheio_id, osWaitForever); // há dado disponível?
    state = buffer[index_c]; // retira do buffer
    osSemaphoreRelease(vazio_id); // sinaliza um espaço a mais
    
    index_c++;
    if(index_c >= BUFFER_SIZE) // incrementa índice de retirada do buffer
      index_c = 0;
    
    LEDWrite(LED4 | LED3 | LED2 | LED1, state); // apresenta informação consumida
    osDelay(500);
  } // while
} // consumidor

void main(void){
  SystemInit();
  LEDInit(LED4 | LED3 | LED2 | LED1);
  ButtonInit(USW1);
  ButtonIntEnable(USW1);
  
  osKernelInitialize();

  consumidor_id = osThreadNew(consumidor, NULL, NULL);

  vazio_id = osSemaphoreNew(BUFFER_SIZE, BUFFER_SIZE, NULL); // espaços disponíveis = BUFFER_SIZE
  cheio_id = osSemaphoreNew(BUFFER_SIZE, 0, NULL); // espaços ocupados = 0
  
  if(osKernelGetState() == osKernelReady)
    osKernelStart();

  while(1);
} // main
