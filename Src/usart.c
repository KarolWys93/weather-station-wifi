/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */
#include <stdbool.h>
#include <string.h>

static const uint8_t newLinePattern[] = {'\r', '\n'};
static const uint8_t patternSize = sizeof(newLinePattern);

static volatile uint8_t uart_line_mode = false;

/* USER CODE END 0 */

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart1_tx;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart2_tx;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}
/* USART2 init function */

void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PB6     ------> USART1_TX
    PB7     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    __HAL_AFIO_REMAP_USART1_ENABLE();

    /* USART1 DMA Init */
    /* USART1_TX Init */
    hdma_usart1_tx.Instance = DMA1_Channel4;
    hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_tx.Init.Mode = DMA_NORMAL;
    hdma_usart1_tx.Init.Priority = DMA_PRIORITY_HIGH;
    if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart1_tx);

    /* USART1_RX Init */
    hdma_usart1_rx.Instance = DMA1_Channel5;
    hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_rx.Init.Mode = DMA_NORMAL;
    hdma_usart1_rx.Init.Priority = DMA_PRIORITY_HIGH;
    if (HAL_DMA_Init(&hdma_usart1_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart1_rx);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

  /* USER CODE END USART1_MspInit 1 */
  }
  else if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspInit 0 */

  /* USER CODE END USART2_MspInit 0 */
    /* USART2 clock enable */
    __HAL_RCC_USART2_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART2 DMA Init */
    /* USART2_TX Init */
    hdma_usart2_tx.Instance = DMA1_Channel7;
    hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_tx.Init.Mode = DMA_NORMAL;
    hdma_usart2_tx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_usart2_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart2_tx);

    /* USART2 interrupt Init */
    HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspInit 1 */

  /* USER CODE END USART2_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PB6     ------> USART1_TX
    PB7     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6|GPIO_PIN_7);

    /* USART1 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmatx);
    HAL_DMA_DeInit(uartHandle->hdmarx);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspDeInit 0 */

  /* USER CODE END USART2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART2_CLK_DISABLE();

    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);

    /* USART2 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmatx);

    /* USART2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspDeInit 1 */

  /* USER CODE END USART2_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

uint8_t UART_isLineModeOn(void)
{
	return uart_line_mode;
}

HAL_StatusTypeDef UART_TransmitLine(UART_HandleTypeDef *huart, const char* text, uint32_t Timeout)
{
	HAL_StatusTypeDef status = HAL_UART_Transmit(huart, (uint8_t *)text, strlen(text), Timeout);
	if(HAL_OK == status) { status = HAL_UART_Transmit(huart, (uint8_t*)newLinePattern, patternSize, Timeout); }
	return status;
}

HAL_StatusTypeDef UART_ReceiveLine(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	uint32_t tickstart = 0U;
	uint8_t currentChar = '\0';
	bool newLineIsReady = false;
	uint16_t patternCounter = 0;

	if(huart->RxState != HAL_UART_STATE_READY)
	{
		return HAL_BUSY;
	}
	else
	{
		if ((pData == NULL) || (Size == 0U))
	    {
	      return  HAL_ERROR;
	    }

	    /* Process Locked */
	    __HAL_LOCK(huart);
		huart->RxState = HAL_UART_STATE_BUSY_RX;
		huart->ErrorCode = HAL_UART_ERROR_NONE;

		huart->RxXferSize = Size;
		huart->RxXferCount = 0;
		huart->pRxBuffPtr = pData;
		uart_line_mode = true;

	    /* Process Unlocked */
	    __HAL_UNLOCK(huart);

	    tickstart = HAL_GetTick();
	    currentChar = (uint8_t)(huart->Instance->DR);	//dumb read for flag clear
	    while(false == newLineIsReady)
	    {
	    	//wait for char
	    	while(!__HAL_UART_GET_FLAG(huart, UART_FLAG_RXNE))
	    	{
	    	    if (Timeout != HAL_MAX_DELAY)
	    	    {
	    	      if ((Timeout == 0U) || ((HAL_GetTick() - tickstart) > Timeout))
	    	      {
	    	          huart->gState  = HAL_UART_STATE_READY;
	    	          huart->RxState = HAL_UART_STATE_READY;
	    	          __HAL_UNLOCK(huart);
	    	          uart_line_mode = false;
	    	          return HAL_TIMEOUT;
	    	      }
	    	    }
	    	}

	    	currentChar = (uint8_t)(huart->Instance->DR);
			*(huart->pRxBuffPtr + huart->RxXferCount) = currentChar;
			huart->RxXferCount++;

	    	patternCounter = (currentChar == newLinePattern[patternCounter])? (patternCounter + 1) : 0;

	    	if(patternCounter >= patternSize)
	    	{
	    		newLineIsReady = true;
	    		patternCounter = 0;
	    		huart->RxXferCount-=(patternSize - 1);
	    		*(huart->pRxBuffPtr + (huart->RxXferCount - 1)) = '\0';
	    	}

   			if(huart->RxXferCount >= huart->RxXferSize)
   			{
   				newLineIsReady = true;
	    		*(huart->pRxBuffPtr + (huart->RxXferCount - 1)) = '\0';
	    	}
	    }

	    huart->RxState = HAL_UART_STATE_READY;
		uart_line_mode = false;
	}
	return HAL_OK;
}

HAL_StatusTypeDef UART_ReceiveLine_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
	if (huart->RxState != HAL_UART_STATE_READY)
	{
		return HAL_BUSY;
	}
	else
	{
	    if ((pData == NULL) || (Size == 0U))
	    {
	      return HAL_ERROR;
	    }
		/* Process Locked */
		__HAL_LOCK(huart);

		huart->RxState = HAL_UART_STATE_BUSY_RX;
		huart->ErrorCode = HAL_UART_ERROR_NONE;

		huart->RxXferSize = Size;
		huart->RxXferCount = 0;
		huart->pRxBuffPtr = pData;

		uart_line_mode = true;

		/* Process Unlocked */
		__HAL_UNLOCK(huart);

		/* Remove any pending event */
		__HAL_UART_CLEAR_NEFLAG(huart);
		/* Enable the UART Data Register not empty Interrupt */
		__HAL_UART_ENABLE_IT(huart, UART_IT_RXNE);

		return HAL_OK;
	}
}

__weak void UART_NewLineReceivedCallback(UART_HandleTypeDef *huart)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(huart);

	/* NOTE : This function should not be modified, when the callback is needed,
            the UART_NewLineReceivedCallback can be implemented in the user file.
	 */
}

HAL_StatusTypeDef UART_AbortReceiveLine_IT(UART_HandleTypeDef *huart)
{
	/* Disable the UART Data Register not empty Interrupt */
	__HAL_UART_DISABLE_IT(huart, UART_IT_RXNE);
	/* Rx process is completed, restore huart->RxState to Ready */
	huart->RxState = HAL_UART_STATE_READY;
	uart_line_mode = false;
	return HAL_OK;
}

void UART_LineModeIRQHandler(UART_HandleTypeDef *huart)
{
	uint32_t isrflags   = READ_REG(huart->Instance->SR);
	uint32_t cr1its     = READ_REG(huart->Instance->CR1);

	uint8_t currentChar = '\0';
	bool newLineIsReady = false;

	static uint16_t patternCounter = 0;

	/* UART in mode Receiver -------------------------------------------------*/
	if (((isrflags & USART_SR_RXNE) != RESET) && ((cr1its & USART_CR1_RXNEIE) != RESET))
	{
		currentChar = (uint8_t)(huart->Instance->DR);

		*(huart->pRxBuffPtr + huart->RxXferCount) = currentChar;
		huart->RxXferCount++;

		patternCounter = (currentChar == newLinePattern[patternCounter])? (patternCounter + 1) : 0;
		if(patternCounter >= patternSize)
		{
			newLineIsReady = true;
			patternCounter = 0;
			huart->RxXferCount-=(patternSize - 1);
			*(huart->pRxBuffPtr + (huart->RxXferCount - 1)) = '\0';
		}


		if(huart->RxXferCount >= huart->RxXferSize)
		{
			newLineIsReady = true;
			*(huart->pRxBuffPtr + (huart->RxXferCount - 1)) = '\0';
		}


		if(true == newLineIsReady)
		{
			/* Disable the UART Data Register not empty Interrupt */
			__HAL_UART_DISABLE_IT(huart, UART_IT_RXNE);

			/* Rx process is completed, restore huart->RxState to Ready */
			huart->RxState = HAL_UART_STATE_READY;
			uart_line_mode = false;
			UART_NewLineReceivedCallback(huart);
		}
	}
}

#ifdef __GNUC__
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
/**
 * @brief  Retargets the C library printf function to the USART.
 * @param  None
 * @retval None
 */
PUTCHAR_PROTOTYPE
{
	/* Place your implementation of fputc here */
	/* e.g. write a character to the EVAL_COM1 and Loop until the end of transmission */
	HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);

	return ch;
}
/* USER CODE END 1 */
