/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "fatfs.h"
#include "rtc.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "wifi/wifi_esp.h"
#include "system.h"
#include "jsmn.h"
#include "app/server/httpServerApp.h"
#include "app/client/forecast_client.h"
#include "logger.h"
#include "utils.h"

#include "led.h"
#include "images.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
char deviceName[32] = "";
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

Wifi_RespStatus loadWiFiAPConfig(void)
{
	SWiFi_AP_Config ap_config;
	FIL configFile;

	char apConfigStr[256];
	uint16_t apConfigSize = 0;

	jsmn_parser jsonParser;
	jsmntok_t jsonTokens[10];
	int16_t numOfTokens;

	uint16_t tokenSize;
	char* tokenPtr;

	memset(&ap_config, 0, sizeof(SWiFi_AP_Config));

	jsmn_init(&jsonParser);

	if(FR_OK == f_open(&configFile, FILE_PATH_AP_CONFIG, FA_READ))
	{
		if(FR_OK != f_read(&configFile, apConfigStr, sizeof(apConfigStr), (UINT*)&apConfigSize))
		{
			f_close(&configFile);
			Logger(LOG_ERR, "Cannot read AP config");
			return WIFI_RESP_ERROR;
		}
		f_close(&configFile);
	}
	else
	{
		Logger(LOG_ERR, "Cannot open AP config");
		return WIFI_RESP_ERROR;
	}

	numOfTokens = jsmn_parse(&jsonParser, apConfigStr, apConfigSize, jsonTokens, sizeof(jsonTokens) / sizeof(jsonTokens[0]));
	if(0 > numOfTokens || jsonTokens[0].type != JSMN_OBJECT)	//if no tokens or first token isn't object
	{
		Logger(LOG_ERR, "AP config corrupted");
		return WIFI_RESP_ERROR;
	}

	for(uint16_t i = 1; i < numOfTokens; i++)
	{
		if(0 == jsmn_eq(apConfigStr, &jsonTokens[i], "ssid"))
		{
			tokenSize = jsonTokens[i + 1].end - jsonTokens[i + 1].start;
			tokenPtr = apConfigStr + jsonTokens[i + 1].start;
			memcpy(ap_config.ssid, tokenPtr, tokenSize);
			i++;
		}
		else if(0 == jsmn_eq(apConfigStr, &jsonTokens[i], "pass"))
		{
			tokenSize = jsonTokens[i + 1].end - jsonTokens[i + 1].start;
			tokenPtr = apConfigStr + jsonTokens[i + 1].start;
			memcpy(ap_config.pass, tokenPtr, tokenSize);
			i++;
		}
		else if(0 == jsmn_eq(apConfigStr, &jsonTokens[i], "enc"))
		{
			WiFi_Encryption encVal = (WiFi_Encryption)atoi(apConfigStr + jsonTokens[i + 1].start);
			ap_config.enc = encVal;
			i++;
		}
		else if(0 == jsmn_eq(apConfigStr, &jsonTokens[i], "channel"))
		{
			uint8_t channelVal = atoi(apConfigStr + jsonTokens[i + 1].start);
			ap_config.channel = channelVal;
			i++;
		}
		else
		{
			i++;
		}
	}

	ap_config.maxConnection = 1;

	return WiFi_AP_mode(&ap_config);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_RTC_Init();
  MX_SPI2_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_FATFS_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint16_t retryCounter = 0;

  led_setColor(LED_GREEN);
  system_init();

  if(system_batteryLevel() == 0 && SYSTEM_POWER_BATTERY == system_powerStatus())
  {
	  show_low_bat_image();
	  system_shutdown();
  }

  //WiFi restart procedure
  while (1)
  {
	  if(WIFI_RESP_OK == WiFi_restart(5000))
	  {
		  char WiFi_verString[10];

		  Logger(LOG_INF, "WiFi restart OK!");
		  WiFi_setStationName(system_getHostName(), 1000);
		  if(WIFI_RESP_OK == WiFi_GetVersionString(WiFi_verString, 20, 1000))
		  {
			  Logger(LOG_VIP, "WiFi version: %s", WiFi_verString);
		  }
		  break;
	  }
	  else
	  {
		  retryCounter++;
		  Logger(LOG_ERR, "WiFi restart failed %d times", retryCounter);
		  if(retryCounter > 5)
		  {
			  WiFi_shutdown();
			  show_error_image(ERR_IMG_GENERAL, "WIFI error");
			  for(uint8_t i = 0; i < 20; i++)
			  {
				  led_setColor(LED_RED);
				  system_sleep(250);
				  led_setColor(LED_OFF);
				  system_sleep(250);
			  }
			  system_shutdown();
		  }
		  WiFi_shutdown();
		  system_sleep(1000);
	  }
  }

  Logger(LOG_INF, "Set SNTP config");
  WiFi_setSNTPconfig(1, 0, 1000);
  system_sleep(1000);

  //start applications
  if(system_isConfigModeOn())
  {
	  f_unlink(FILE_PATH_CONFIG_MODE_FLAG);

	  if(WIFI_RESP_OK == loadWiFiAPConfig())
	  {
		  Logger(LOG_VIP, "Enter config mode");
		  show_configMode_image();

		  int8_t serverExitCode = 0;
		  Logger(LOG_INF, "Start server");
		  if(0 != (serverExitCode = runServerApp(80, 5, 1000)))
		  {
			  Logger(LOG_ERR, "Server exit with code %d", serverExitCode);
			  show_error_image(ERR_IMG_GENERAL, "server fatal");
		  }
	  }
	  else
	  {
		  show_error_image(ERR_IMG_GENERAL, "WIFI AP error");
	  }
  }
  else
  {
	  runForecastApp();
  }

  system_shutdown();	//set system in standby mode

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_ADC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
