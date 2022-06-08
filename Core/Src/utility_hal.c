/*
 * utility_hal.c
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


BUFF_BUFFER_t * buff;
BUFF_BUFFER_t * buff_rx;
#define buffer_SIZE     512
uint8_t buffer_DMA_1[buffer_SIZE];
uint8_t buffer_DMA_2[buffer_SIZE];


unsigned portBASE_TYPE makeFreeRtosPriority (osPriority priority)
{
unsigned portBASE_TYPE fpriority = tskIDLE_PRIORITY;

if (priority != osPriorityError) {
fpriority += (priority - osPriorityIdle);
}

return fpriority;
}

int _write(int file, char *ptr, int len){
    int DataIdx;
    for (DataIdx = 0; DataIdx < len; DataIdx++){
       //ITM_SendChar( ptr++ );
       HAL_UART_Transmit(&huart2, ptr,len,1000);
    }
    return len;
}


static void DMA_SetConfig(DMA_HandleTypeDef *hdma, uint32_t SrcAddress, uint32_t DstAddress, uint32_t DataLength)
{
  /* Clear DBM bit */
  hdma->Instance->CR &= (uint32_t)(~DMA_SxCR_DBM);

  /* Configure DMA Stream data length */
  hdma->Instance->NDTR = DataLength;

  /* Memory to Peripheral */
  if((hdma->Init.Direction) == DMA_MEMORY_TO_PERIPH)
  {
    /* Configure DMA Stream destination address */
    hdma->Instance->PAR = DstAddress;

    /* Configure DMA Stream source address */
    hdma->Instance->M0AR = SrcAddress;
  }
  /* Peripheral to Memory */
  else
  {
    /* Configure DMA Stream source address */
    hdma->Instance->PAR = SrcAddress;

    /* Configure DMA Stream destination address */
    hdma->Instance->M0AR = DstAddress;
  }
}

/* Private types -------------------------------------------------------------*/
typedef struct
{
  __IO uint32_t ISR;   /*!< DMA interrupt status register */
  __IO uint32_t Reserved0;
  __IO uint32_t IFCR;  /*!< DMA interrupt flag clear register */
} DMA_Base_Registers;

/**
  * @brief Stops the DMA Transfer.
  * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval HAL status
  */

static void UART_EndRxTransfer(UART_HandleTypeDef *huart)
{
  /* Disable RXNE, PE and ERR (Frame error, noise error, overrun error) interrupts */
  ATOMIC_CLEAR_BIT(huart->Instance->CR1, (USART_CR1_RXNEIE | USART_CR1_PEIE));
  ATOMIC_CLEAR_BIT(huart->Instance->CR3, USART_CR3_EIE);

  /* In case of reception waiting for IDLE event, disable also the IDLE IE interrupt source */
  if (huart->ReceptionType == HAL_UART_RECEPTION_TOIDLE)
  {
    ATOMIC_CLEAR_BIT(huart->Instance->CR1, USART_CR1_IDLEIE);
  }

  /* At end of Rx process, restore huart->RxState to Ready */
  huart->RxState = HAL_UART_STATE_READY;
  huart->ReceptionType = HAL_UART_RECEPTION_STANDARD;
}


/**
  * @brief  Aborts the DMA Transfer.
  * @param  hdma   pointer to a DMA_HandleTypeDef structure that contains
  *                 the configuration information for the specified DMA Stream.
  *
  * @note  After disabling a DMA Stream, a check for wait until the DMA Stream is
  *        effectively disabled is added. If a Stream is disabled
  *        while a data transfer is ongoing, the current data will be transferred
  *        and the Stream will be effectively disabled only after the transfer of
  *        this single data is finished.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_DMA_Abort_PAS(DMA_HandleTypeDef *hdma)
{
  /* calculate DMA base and stream number */
  DMA_Base_Registers *regs = (DMA_Base_Registers *)hdma->StreamBaseAddress;

  //uint32_t tickstart = HAL_GetTick();

  if(hdma->State != HAL_DMA_STATE_BUSY)
  {
    hdma->ErrorCode = HAL_DMA_ERROR_NO_XFER;

    /* Process Unlocked */
    __HAL_UNLOCK(hdma);

    return HAL_ERROR;
  }
  else
  {
    /* Disable all the transfer interrupts */
    hdma->Instance->CR  &= ~(DMA_IT_TC | DMA_IT_TE | DMA_IT_DME);
    hdma->Instance->FCR &= ~(DMA_IT_FE);

    if((hdma->XferHalfCpltCallback != NULL) || (hdma->XferM1HalfCpltCallback != NULL))
    {
      hdma->Instance->CR  &= ~(DMA_IT_HT);
    }

    /* Disable the stream */
    __HAL_DMA_DISABLE(hdma);

    /* Check if the DMA Stream is effectively disabled */
    /*PASCUAL *//*while((hdma->Instance->CR & DMA_SxCR_EN) != RESET)
    {
      if((HAL_GetTick() - tickstart ) > HAL_TIMEOUT_DMA_ABORT)
      {
        hdma->ErrorCode = HAL_DMA_ERROR_TIMEOUT;

        hdma->State = HAL_DMA_STATE_TIMEOUT;

        __HAL_UNLOCK(hdma);

        return HAL_TIMEOUT;
      }
    }*/

    /* Clear all interrupt flags at correct offset within the register */
    regs->IFCR = 0x3FU << hdma->StreamIndex;

    /* Change the DMA state*/
    hdma->State = HAL_DMA_STATE_READY;

    /* Process Unlocked */
    __HAL_UNLOCK(hdma);
  }
  return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_DMAStop_PAS(UART_HandleTypeDef *huart)
{
  uint32_t dmarequest = 0x00U;
  /* The Lock is not implemented on this API to allow the user application
     to call the HAL UART API under callbacks HAL_UART_TxCpltCallback() / HAL_UART_RxCpltCallback():
     when calling HAL_DMA_Abort() API the DMA TX/RX Transfer complete interrupt is generated
     and the correspond call back is executed HAL_UART_TxCpltCallback() / HAL_UART_RxCpltCallback()
     */


  /* Stop UART DMA Rx request if ongoing */
  dmarequest = HAL_IS_BIT_SET(huart->Instance->CR3, USART_CR3_DMAR);
  if ((huart->RxState == HAL_UART_STATE_BUSY_RX) && dmarequest)
  {
    ATOMIC_CLEAR_BIT(huart->Instance->CR3, USART_CR3_DMAR);

    /* Abort the UART DMA Rx stream */
    if (huart->hdmarx != NULL)
    {
      HAL_DMA_Abort_PAS(huart->hdmarx);
    }
    UART_EndRxTransfer(huart);
  }

  return HAL_OK;
}



uint32_t HAL_DMA_getcounter(UART_HandleTypeDef *huart){
	return huart->hdmarx->Instance->NDTR;}


