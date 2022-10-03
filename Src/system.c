/*
 * system.c
 *
 *  Created on: 27.04.2022
 *      Author: Karol
 */

#include "system.h"
#include "fatfs.h"
#include "logger.h"
#include <string.h>
#include "main.h"
#include "rtc.h"
#include "adc.h"

#include "wifi/wifi_esp.h"

#include "e-Paper/EPD_1in54b.h"
#include "Config/DEV_Config.h"
#include "GUI/GUI_Paint.h"
#include "e-Paper/ImageData.h"

#include "images.h"

#define BCKUP_REGISTER_WKUP_CNT 1

typedef enum system_rst_src
{
	SYSTEM_PWR_RST = 0,
	SYSTEM_WAKEUP_RST,
	SYSTEM_ALARM_RST
} system_rst_src;

typedef struct SSystemConfig
{
	uint8_t configMode;
	uint8_t ledIndON;
	uint32_t wakeUpCounter;
	system_rst_src resetSrc;
	char hostName[33];
} SSystemConfig;


volatile uint8_t system_rtc_alarm_on = 0;

static FATFS fs;
static SSystemConfig systemConfig;

static uint32_t startTimestamp = 0;

//power source
#define BAT_VOLTAGE_100    4000 //4000 mV
#define BAT_VOLTAGE_0      3330 //3330 mV

#define BAT_VOLTAGE_SAMPLES 8
#define BAT_VOLT_FILTER_DATA 8

static volatile uint16_t batteryAdcData[BAT_VOLTAGE_SAMPLES*2];
static volatile uint32_t batteryVoltage = UINT32_MAX;
static volatile uint8_t bat_adc_is_runing = 0;

//private functions
static void shutdownSystem(void);
static void cardMountFailed(void);
static void startBatteryMeasurment(void);
static uint32_t filterVoltageValue(uint32_t voltage);


void system_init(void)
{
	FIL file;
	int32_t toEarly = 0;

	startTimestamp = HAL_GetTick();

	memset(&systemConfig, 0, sizeof(SSystemConfig));

	if(__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET)
	{
		/* Clear standby flag */
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);

		/* Clear wakeup flag */
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

		systemConfig.wakeUpCounter = HAL_RTCEx_BKUPRead(NULL, BCKUP_REGISTER_WKUP_CNT);
		systemConfig.wakeUpCounter++;
		systemConfig.resetSrc = SYSTEM_WAKEUP_RST;

		if(RTC_getAlarmTime() <= RTC_getTime())
		{
			systemConfig.resetSrc = SYSTEM_ALARM_RST;
		}

		toEarly = RTC_getTime() - RTC_getAlarmTime();
		if(toEarly < 0) toEarly = 0;
	}

//	if(system_rtc_alarm_on)
//	{
//		systemConfig.resetSrc = SYSTEM_ALARM_RST;
//	}

	HAL_RTCEx_BKUPWrite(NULL, BCKUP_REGISTER_WKUP_CNT, systemConfig.wakeUpCounter);

	if(__HAL_PWR_GET_FLAG(PWR_CSR_EWUP) == RESET)
	{
		HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
	}

	HAL_ADCEx_Calibration_Start(&hadc1);
	startBatteryMeasurment();

	/* Mount SD Card */
	if(FR_OK != f_mount(&fs, "", 1))
	{
		cardMountFailed();
	}
	system_sleep(100);

	Logger_init();
	Logger(LOG_VIP, "Reset cause: %d", systemConfig.resetSrc);
	Logger(LOG_VIP, "Version: %s", VERSION_STR);

	Logger(LOG_VIP, "toEarly: %d", toEarly);

	#ifdef DEBUG
	Logger_setMinLevel(LOG_DBG);
	#endif

	//read configs
	if(FR_OK == f_stat(FILE_PATH_LED_IND_FLAG, NULL))
	{
		systemConfig.ledIndON = 1;
	}

	if(FR_OK == f_open(&file, FILE_PATH_HOSTNAME, FA_READ))
	{
		if(NULL == f_gets(systemConfig.hostName, 33, &file))
		{
			systemConfig.hostName[0] = '\0';
		}
		f_close(&file);
	}

	if(GPIO_PIN_RESET == HAL_GPIO_ReadPin(CONFIG_MODE_GPIO_Port, CONFIG_MODE_Pin))
	{
		systemConfig.configMode += 1;
	}

	if(FR_OK == f_stat(FILE_PATH_CONFIG_MODE_FLAG, NULL))
	{
		systemConfig.configMode += 2;
		f_unlink(FILE_PATH_CONFIG_MODE_FLAG);
	}

	Logger(LOG_INF, "Config mode %d", systemConfig.configMode);

	if(EPD_Init())
	{
		Logger(LOG_ERR, "e-Paper init failed");
	}

	if(systemConfig.wakeUpCounter % 5 == 0)
	{
		Logger(LOG_INF, "clear disp");
		EPD_Clear();
	}
	Logger(LOG_INF, "system init OK (%d)", systemConfig.wakeUpCounter);
}

void system_shutdown(void)
{
	shutdownSystem();
	HAL_PWR_EnterSTANDBYMode();
}

void system_restart(uint8_t configMode)
{
	Logger(LOG_INF, "Prepare for restart in %s mode", configMode ? "normal" : "config");
	if(configMode)
	{
		FIL file;
		if(FR_OK == f_open(&file, FILE_PATH_CONFIG_MODE_FLAG, FA_OPEN_ALWAYS))
		{
			f_close(&file);
		}
	}

	shutdownSystem();
	NVIC_SystemReset();
}

void system_setWakeUpTimer(uint32_t seconds)
{
	  time_t timeAlarm = RTC_getTime();
	  timeAlarm += seconds;
	  Logger(LOG_INF, "Wake-up signal in %d seconds", seconds);

	  RTC_setAlarmTime(timeAlarm);
}

char* const system_getHostName(void)
{
	return systemConfig.hostName;
}

uint8_t system_isLedIndicatorOn(void)
{
	return systemConfig.ledIndON;
}

uint8_t system_isConfigModeOn(void)
{
	return systemConfig.configMode;
}

uint8_t system_isCharging(void)
{
	if(GPIO_PIN_SET == HAL_GPIO_ReadPin(BAT_CHR_GPIO_Port, BAT_CHR_Pin))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

uint32_t system_batteryVoltage(void)
{
	if(!bat_adc_is_runing)
	{
		startBatteryMeasurment();
	}
	return batteryVoltage;
}

uint8_t system_batteryLevel(void)
{
	uint32_t batteryVoltage = system_batteryVoltage();
	uint32_t tmpVoltage;
	uint8_t batteryLevel = 0xFF;

	if(batteryVoltage == UINT32_MAX)
	{
		return batteryLevel;
	}

	tmpVoltage = batteryVoltage;

	if(tmpVoltage > BAT_VOLTAGE_100)
	{
		tmpVoltage = BAT_VOLTAGE_100;
	}

	if(tmpVoltage < BAT_VOLTAGE_0)
	{
		tmpVoltage = BAT_VOLTAGE_0;
	}

	batteryLevel = ((tmpVoltage-BAT_VOLTAGE_0) * 100) / (BAT_VOLTAGE_100 - BAT_VOLTAGE_0);

	Logger(LOG_INF, "Battery: %lu mV, %u%%, (%u)", batteryVoltage, batteryLevel, system_isCharging());

	return batteryLevel;
}

void system_sleep(uint32_t miliseconds)
{
  uint32_t tickstart = HAL_GetTick();
  uint32_t wait = miliseconds;

  /* Add a freq to guarantee minimum wait */
  if (wait < HAL_MAX_DELAY)
  {
    wait += (uint32_t)(uwTickFreq);
  }

  while ((HAL_GetTick() - tickstart) < wait)
  {
	  HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
  }
}

uint8_t system_restoreDefault(void)
{
	FIL file;
	int8_t result = 0;

	Logger(LOG_WRN, "restore default settings");

	//create directories
	f_mkdir(DIR_PATH_SYS);
	f_mkdir(DIR_PATH_SYS);

	//WiFi config
	f_unlink(FILE_PATH_WIFI_CONFIG);

	//forecast settings
	f_unlink(FILE_PATH_FORECAST_CONFIG);

	//led indicator
	if(0 == result)
	{
		if(FR_OK == f_open(&file, FILE_PATH_LED_IND_FLAG, FA_OPEN_ALWAYS))
		{
			f_close(&file);
		}
		else
		{
			result = 1;
		}
	}

	//host name
	if(0 == result)
	{
		if(FR_OK == f_open(&file, FILE_PATH_HOSTNAME, FA_OPEN_ALWAYS | FA_WRITE))
		{
			f_printf(&file, "WeatherStation_%04X", (int)HAL_GetDEVID());
			f_close(&file);
		}
		else
		{
			result = 1;
		}
	}

	//access point config
	if(0 == result)
	{
		if(FR_OK == f_open(&file, FILE_PATH_AP_CONFIG, FA_OPEN_ALWAYS | FA_WRITE))
		{
			f_printf(&file, "{\"ssid\":\"WeatherStation_%04X\",\"enc\":3,\"channel\":8,\"pass\":\"littlefox123\"}", (int)HAL_GetDEVID());
			f_close(&file);
		}
		else
		{
			result = 1;
		}
	}
	return result;
}

static void shutdownSystem(void)
{
	Logger(LOG_INF, "System shutdown");
	EPD_Sleep();
	WiFi_DisconnectStation(1000);

	Logger(LOG_INF, "Run time: %d ms", HAL_GetTick()-startTimestamp);
	Logger_shutdown();
}

static void cardMountFailed(void)
{
	Logger(LOG_VIP, "SD card err");
	EPD_Init();
	show_error_image(ERR_IMG_MEMORY_CARD, "SD card err");
	EPD_Sleep();
	HAL_PWR_EnterSTANDBYMode();
}

static void startBatteryMeasurment(void)
{
	if(!system_isCharging())
	{
		HAL_GPIO_WritePin(BAT_ADC_GND_GPIO_Port, BAT_ADC_GND_Pin, GPIO_PIN_RESET);
		HAL_ADC_Start_DMA(&hadc1, (uint32_t*) batteryAdcData, BAT_VOLTAGE_SAMPLES*2);
		bat_adc_is_runing = 1;
	}
}

static uint32_t filterVoltageValue(uint32_t voltage)
{
	static volatile bool isInitiated = false;
	static volatile uint32_t batteryVoltageHistory[BAT_VOLT_FILTER_DATA];
	static volatile uint8_t index = 0;

	if(!isInitiated)
	{
		for(uint8_t i = 0; i < BAT_VOLT_FILTER_DATA; i++)
		{
			batteryVoltageHistory[i] = voltage;
		}
		isInitiated = true;
		return voltage;
	}
	else
	{
		uint32_t tmp = 0;
		batteryVoltageHistory[index++] = voltage;
		index %= BAT_VOLT_FILTER_DATA;

		for(uint8_t i = 0; i < BAT_VOLT_FILTER_DATA; i++)
		{
			tmp += batteryVoltageHistory[i];
		}

		return tmp/BAT_VOLT_FILTER_DATA;
	}
}

//battery voltage measurment callback
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	// R1 = 10 kOhm, R2 = 33 kOhm
	const uint32_t divFacttor1 = 33;    //R2
	const uint32_t divFacttor2 = 43;    //R1+R2
	uint32_t batteryADC = 0;
	uint32_t refVoltADC = 0;

	HAL_GPIO_WritePin(BAT_ADC_GND_GPIO_Port, BAT_ADC_GND_Pin, GPIO_PIN_SET);

	for(uint8_t i = 0; i < BAT_VOLTAGE_SAMPLES*2; i+=2)
	{
		batteryADC += batteryAdcData[i];
		refVoltADC += batteryAdcData[i+1];
	}

	batteryADC /= BAT_VOLTAGE_SAMPLES;
	refVoltADC /= BAT_VOLTAGE_SAMPLES;

	//vref -> 1200 mV
	batteryVoltage = ((batteryADC * 1200)/refVoltADC);

	//voltage divider
	batteryVoltage = (batteryVoltage*divFacttor2)/divFacttor1;

	batteryVoltage = filterVoltageValue(batteryVoltage);

	bat_adc_is_runing = 0;
}
