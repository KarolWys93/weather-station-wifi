/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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
#define LED_G_Pin GPIO_PIN_13
#define LED_G_GPIO_Port GPIOC
#define LED_R_Pin GPIO_PIN_14
#define LED_R_GPIO_Port GPIOC
#define LED_B_Pin GPIO_PIN_15
#define LED_B_GPIO_Port GPIOC
#define SYS_WKUP_Pin GPIO_PIN_0
#define SYS_WKUP_GPIO_Port GPIOA
#define CONFIG_MODE_Pin GPIO_PIN_4
#define CONFIG_MODE_GPIO_Port GPIOA
#define D_CLK_Pin GPIO_PIN_5
#define D_CLK_GPIO_Port GPIOA
#define FACTORY_RST_Pin GPIO_PIN_6
#define FACTORY_RST_GPIO_Port GPIOA
#define D_DIN_Pin GPIO_PIN_7
#define D_DIN_GPIO_Port GPIOA
#define D_CS_Pin GPIO_PIN_0
#define D_CS_GPIO_Port GPIOB
#define BAT_ADC_Pin GPIO_PIN_1
#define BAT_ADC_GPIO_Port GPIOB
#define D_DC_Pin GPIO_PIN_2
#define D_DC_GPIO_Port GPIOB
#define D_RST_Pin GPIO_PIN_10
#define D_RST_GPIO_Port GPIOB
#define D_BUSY_Pin GPIO_PIN_11
#define D_BUSY_GPIO_Port GPIOB
#define SD_CS_Pin GPIO_PIN_12
#define SD_CS_GPIO_Port GPIOB
#define SD_CLK_Pin GPIO_PIN_13
#define SD_CLK_GPIO_Port GPIOB
#define SD_MISO_Pin GPIO_PIN_14
#define SD_MISO_GPIO_Port GPIOB
#define SD_MOSI_Pin GPIO_PIN_15
#define SD_MOSI_GPIO_Port GPIOB
#define SD_PWR_Pin GPIO_PIN_8
#define SD_PWR_GPIO_Port GPIOA
#define ESP_TX_Pin GPIO_PIN_9
#define ESP_TX_GPIO_Port GPIOA
#define ESP_RX_Pin GPIO_PIN_10
#define ESP_RX_GPIO_Port GPIOA
#define BAT_ADC_GND_Pin GPIO_PIN_11
#define BAT_ADC_GND_GPIO_Port GPIOA
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define USB_DET_Pin GPIO_PIN_7
#define USB_DET_GPIO_Port GPIOB
#define BAT_CHR_Pin GPIO_PIN_8
#define BAT_CHR_GPIO_Port GPIOB
#define ESP_EN_Pin GPIO_PIN_9
#define ESP_EN_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
