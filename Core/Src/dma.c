/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    dma.c
  * @brief   This file provides code for the configuration
  *          of all the requested memory to memory DMA transfers.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "dma.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure DMA                                                              */
/*----------------------------------------------------------------------------*/

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
DMA_HandleTypeDef hdma_memtomem_dma1_channel2;

/**
  * Enable DMA controller clock
  * Configure DMA for memory to memory transfers
  *   hdma_memtomem_dma1_channel2
  */
void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* Configure DMA request hdma_memtomem_dma1_channel2 on DMA1_Channel2 */
  hdma_memtomem_dma1_channel2.Instance = DMA1_Channel2;
  hdma_memtomem_dma1_channel2.Init.Direction = DMA_MEMORY_TO_MEMORY;
  hdma_memtomem_dma1_channel2.Init.PeriphInc = DMA_PINC_ENABLE;
  hdma_memtomem_dma1_channel2.Init.MemInc = DMA_MINC_ENABLE;
  hdma_memtomem_dma1_channel2.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_memtomem_dma1_channel2.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_memtomem_dma1_channel2.Init.Mode = DMA_NORMAL;
  hdma_memtomem_dma1_channel2.Init.Priority = DMA_PRIORITY_LOW;
  if (HAL_DMA_Init(&hdma_memtomem_dma1_channel2) != HAL_OK)
  {
    Error_Handler();
  }

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
  /* DMA1_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);
  /* DMA1_Channel7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);

}

/* USER CODE BEGIN 2 */

void * DMA_memcpy(void* dstPrt, void* srcPtr, uint16_t num)
{
    MODIFY_REG(hdma_memtomem_dma1_channel2.Instance->CCR, DMA_CCR_PINC, DMA_PINC_ENABLE);
    HAL_DMA_Start_IT(&hdma_memtomem_dma1_channel2, (uint32_t)srcPtr, (uint32_t)dstPrt, num);
    while (HAL_DMA_GetState(&hdma_memtomem_dma1_channel2) != HAL_DMA_STATE_READY)
    {
        HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
    }
    __HAL_DMA_DISABLE(&hdma_memtomem_dma1_channel2);
    return dstPrt;
}

void * DMA_memset( void * ptr, uint8_t value, uint16_t num )
{
    uint32_t valueToCpy = value;
    MODIFY_REG(hdma_memtomem_dma1_channel2.Instance->CCR, DMA_CCR_PINC, DMA_PINC_DISABLE);
    HAL_DMA_Start_IT(&hdma_memtomem_dma1_channel2, (uint32_t)(&valueToCpy), (uint32_t)ptr, num);
    while (HAL_DMA_GetState(&hdma_memtomem_dma1_channel2) != HAL_DMA_STATE_READY)
    {
        HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
    }
    __HAL_DMA_DISABLE(&hdma_memtomem_dma1_channel2);
    return ptr;
}

/* USER CODE END 2 */

