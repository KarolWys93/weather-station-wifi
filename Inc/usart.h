/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);

/* USER CODE BEGIN Prototypes */


uint8_t UART_isLineModeOn(void);
HAL_StatusTypeDef UART_TransmitLine(UART_HandleTypeDef *huart, const char* text, uint32_t Timeout);
HAL_StatusTypeDef UART_ReceiveLine(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef UART_ReceiveLine_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
void UART_NewLineReceivedCallback(UART_HandleTypeDef *huart);
HAL_StatusTypeDef UART_AbortReceiveLine_IT(UART_HandleTypeDef *huart);
void UART_LineModeIRQHandler(UART_HandleTypeDef *huart);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

