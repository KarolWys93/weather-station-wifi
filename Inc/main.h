/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "system_info.h"
#include "system.h"

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define CONFIG_MODE_Pin GPIO_PIN_13
#define CONFIG_MODE_GPIO_Port GPIOC
#define D_BUSY_Pin GPIO_PIN_0
#define D_BUSY_GPIO_Port GPIOC
#define D_DC_Pin GPIO_PIN_1
#define D_DC_GPIO_Port GPIOC
#define BAT_ADC_Pin GPIO_PIN_1
#define BAT_ADC_GPIO_Port GPIOA
#define BAT_ADC_GND_Pin GPIO_PIN_4
#define BAT_ADC_GND_GPIO_Port GPIOA
#define LD2_Pin GPIO_PIN_5
#define LD2_GPIO_Port GPIOA
#define D_RST_Pin GPIO_PIN_0
#define D_RST_GPIO_Port GPIOB
#define SPI2_CS_Pin GPIO_PIN_14
#define SPI2_CS_GPIO_Port GPIOB
#define WIFI_RST_Pin GPIO_PIN_8
#define WIFI_RST_GPIO_Port GPIOC
#define SPI1_CS_Pin GPIO_PIN_10
#define SPI1_CS_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define BAT_CHR_Pin GPIO_PIN_8
#define BAT_CHR_GPIO_Port GPIOB
#define WIFI_PWR_Pin GPIO_PIN_9
#define WIFI_PWR_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
