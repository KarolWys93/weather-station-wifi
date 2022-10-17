/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    rtc.c
  * @brief   This file provides code for the configuration
  *          of the RTC instances.
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
#include "rtc.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

/* RTC init function */
void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef DateToUpdate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
  hrtc.Init.OutPut = RTC_OUTPUTSOURCE_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */
  return;
  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0;
  sTime.Minutes = 0;
  sTime.Seconds = 0;

  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  DateToUpdate.WeekDay = RTC_WEEKDAY_MONDAY;
  DateToUpdate.Month = RTC_MONTH_JANUARY;
  DateToUpdate.Date = 1;
  DateToUpdate.Year = 0;

  if (HAL_RTC_SetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspInit 0 */

  /* USER CODE END RTC_MspInit 0 */
    HAL_PWR_EnableBkUpAccess();
    /* Enable BKP CLK enable for backup registers */
    __HAL_RCC_BKP_CLK_ENABLE();
    /* RTC clock enable */
    __HAL_RCC_RTC_ENABLE();
  /* USER CODE BEGIN RTC_MspInit 1 */

  /* USER CODE END RTC_MspInit 1 */
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspDeInit 0 */

  /* USER CODE END RTC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();
  /* USER CODE BEGIN RTC_MspDeInit 1 */

  /* USER CODE END RTC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

static HAL_StatusTypeDef RTC_EnterInitMode(RTC_HandleTypeDef *hrtc)
{
  uint32_t tickstart = 0U;

  tickstart = HAL_GetTick();
  /* Wait till RTC is in INIT state and if Time out is reached exit */
  while ((hrtc->Instance->CRL & RTC_CRL_RTOFF) == (uint32_t)RESET)
  {
    if ((HAL_GetTick() - tickstart) >  RTC_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }

  /* Disable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);


  return HAL_OK;
}

static HAL_StatusTypeDef RTC_ExitInitMode(RTC_HandleTypeDef *hrtc)
{
  uint32_t tickstart = 0U;

  /* Disable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

  tickstart = HAL_GetTick();
  /* Wait till RTC is in INIT state and if Time out is reached exit */
  while ((hrtc->Instance->CRL & RTC_CRL_RTOFF) == (uint32_t)RESET)
  {
    if ((HAL_GetTick() - tickstart) >  RTC_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }

  return HAL_OK;
}


HAL_StatusTypeDef RTC_setTime(time_t time)
{
	  HAL_StatusTypeDef status = HAL_OK;

	  /* Set Initialization mode */
	  if (RTC_EnterInitMode(&hrtc) != HAL_OK)
	  {
	    status = HAL_ERROR;
	  }
	  else
	  {
	    /* Set RTC COUNTER MSB word */
	    WRITE_REG(hrtc.Instance->CNTH, (time >> 16U));
	    /* Set RTC COUNTER LSB word */
	    WRITE_REG(hrtc.Instance->CNTL, (time & RTC_CNTL_RTC_CNT));

	    /* Wait for synchro */
	    if (RTC_ExitInitMode(&hrtc) != HAL_OK)
	    {
	      status = HAL_ERROR;
	    }
	  }

	  return status;
}

time_t RTC_getTime(void)
{
	  uint16_t high1 = 0U, high2 = 0U, low = 0U;
	  uint32_t timecounter = 0U;

	  high1 = READ_REG(hrtc.Instance->CNTH & RTC_CNTH_RTC_CNT);
	  low   = READ_REG(hrtc.Instance->CNTL & RTC_CNTL_RTC_CNT);
	  high2 = READ_REG(hrtc.Instance->CNTH & RTC_CNTH_RTC_CNT);

	  if (high1 != high2)
	  {
	    /* In this case the counter roll over during reading of CNTL and CNTH registers,
	       read again CNTL register then return the counter value */
	    timecounter = (((uint32_t) high2 << 16U) | READ_REG(hrtc.Instance->CNTL & RTC_CNTL_RTC_CNT));
	  }
	  else
	  {
	    /* No counter roll over during reading of CNTL and CNTH registers, counter
	       value is equal to first value of CNTL and CNTH */
	    timecounter = (((uint32_t) high1 << 16U) | low);
	  }

	  return timecounter;
}

HAL_StatusTypeDef RTC_setAlarmTime(time_t time)
{
	  HAL_StatusTypeDef status = HAL_OK;

	  /* Set Initialization mode */
	  if (RTC_EnterInitMode(&hrtc) != HAL_OK)
	  {
	    status = HAL_ERROR;
	  }
	  else
	  {
	    /* Set RTC COUNTER MSB word */
	    WRITE_REG(hrtc.Instance->ALRH, (time >> 16U));
	    /* Set RTC COUNTER LSB word */
	    WRITE_REG(hrtc.Instance->ALRL, (time & RTC_ALRL_RTC_ALR));

	    /* Wait for synchro */
	    if (RTC_ExitInitMode(&hrtc) != HAL_OK)
	    {
	      status = HAL_ERROR;
	    }
	  }

	  return status;
}

time_t RTC_getAlarmTime(void)
{
	  uint16_t high1 = 0U, low = 0U;

	  high1 = READ_REG(hrtc.Instance->ALRH & RTC_CNTH_RTC_CNT);
	  low   = READ_REG(hrtc.Instance->ALRL & RTC_CNTL_RTC_CNT);

	  return (((uint32_t) high1 << 16U) | low);
}


/* USER CODE END 1 */
