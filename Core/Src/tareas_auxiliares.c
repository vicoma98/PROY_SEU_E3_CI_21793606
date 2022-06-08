/*
 * tareas_auxiliares.c
 *
 *  Created on: May 3, 2022
 *      Author: pperez
 */

#include "FreeRTOS.h"
#include <stdio.h>
#include "tareas_serie.h"
#include <task.h>
#include "cmsis_os.h"
#include "main.h"
#include "utility.h"
#include <string.h>
#include <stm32f4xx_hal_dma.h>


extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart2_rx2;

extern BUFF_BUFFER_t * buff;
extern  BUFF_BUFFER_t * buff_rx;
#define buffer_SIZE     512
extern uint8_t buffer_DMA_1[buffer_SIZE];
extern uint8_t buffer_DMA_2[buffer_SIZE];



void Task_DMA( void *pvParameters ){

	uint32_t it,num;
	HAL_StatusTypeDef res;
	uint32_t nbuff;



    hdma_usart2_rx2.Instance = DMA1_Stream7;
    hdma_usart2_rx2.Init.Channel = DMA_CHANNEL_6;
    hdma_usart2_rx2.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart2_rx2.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_rx2.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_rx2.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_rx2.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_rx2.Init.Mode = DMA_NORMAL;
    hdma_usart2_rx2.Init.Priority = DMA_PRIORITY_LOW;
    hdma_usart2_rx2.Init.FIFOMode = DMA_FIFOMODE_DISABLE;



    if (HAL_DMA_Init(&hdma_usart2_rx2) != HAL_OK)
    {
      Error_Handler();
    }

	nbuff=0;
	res=HAL_UART_Receive_DMA(&huart2, buffer_DMA_1,buffer_SIZE); // Para arrancar

	it=0;
	while(1){

		switch (nbuff){
		case 0: 	num=hdma_usart2_rx.Instance->NDTR;
					if (num<buffer_SIZE){
						__disable_irq();
						res=HAL_UART_DMAStop_PAS(&huart2);
					   __HAL_LINKDMA(&huart2,hdmarx,hdma_usart2_rx2);
					   res=HAL_UART_Receive_DMA(&huart2, buffer_DMA_2,buffer_SIZE);
					   __enable_irq();
					   nbuff=1;
					   num=hdma_usart2_rx.Instance->NDTR;
					   res=buff->puts(buff_rx,buffer_DMA_1,buffer_SIZE-num);
					}else
						;

					break;
		case 1:
	    			num=hdma_usart2_rx2.Instance->NDTR;
	    			if (num<buffer_SIZE){
	    				__disable_irq();
	    				res=HAL_UART_DMAStop_PAS(&huart2);
	    				__HAL_LINKDMA(&huart2,hdmarx,hdma_usart2_rx);
	    				res=HAL_UART_Receive_DMA(&huart2, buffer_DMA_1,buffer_SIZE);
	    				__enable_irq();
	    				nbuff=0;
	    				num=hdma_usart2_rx2.Instance->NDTR;
	    				res=buff->puts(buff_rx,buffer_DMA_2,buffer_SIZE-num);
	    			}else
	    				;
	    			break;
		}

		it++;
		vTaskDelay(1/portTICK_RATE_MS );
	}
}


void Task_Display( void *pvParameters ){

	uint32_t it;
	BUFF_ITEM_t car;
	HAL_StatusTypeDef res;

    it=0;
	while(1){

		buff->get(buff,&car);
		res=HAL_UART_Transmit(& huart2,&car,1,100);
		it++;

	}
}

