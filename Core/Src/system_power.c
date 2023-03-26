/*
 * system_power.c
 *
 *  Created on: Mar 21, 2023
 *      Author: Karol
 */

#include "system_power.h"
#include "logger.h"
#include "adc.h"
#include <stdbool.h>

//power source
#define BAT_VOLTAGE_100    4000 //4000 mV
#define BAT_VOLTAGE_0      3330 //3330 mV

#define BAT_VOLTAGE_SAMPLES 8
#define BAT_VOLT_FILTER_DATA 8

static volatile uint16_t batteryAdcData[BAT_VOLTAGE_SAMPLES*2];
static volatile uint32_t batteryVoltage = UINT32_MAX;
static volatile uint8_t bat_adc_is_runing = 0;

//private functions
static void startBatteryMeasurement(void);
static uint32_t filterVoltageValue(uint32_t voltage);

void system_powerInit(void)
{
	HAL_ADCEx_Calibration_Start(&hadc1);
	startBatteryMeasurement();
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

SYSTEM_POWER_STATUS system_powerStatus(void)
{
	uint8_t powerStatus = SYSTEM_POWER_BATTERY;
	if(GPIO_PIN_SET == HAL_GPIO_ReadPin(USB_DET_GPIO_Port, USB_DET_Pin))
	{
		powerStatus = SYSTEM_POWER_DC_ADAPTER;

		if(GPIO_PIN_RESET == HAL_GPIO_ReadPin(BAT_CHR_GPIO_Port, BAT_CHR_Pin))
		{
			powerStatus = SYSTEM_POWER_CHARGING;
		}
	}
	return powerStatus;
}

uint32_t system_batteryVoltage(void)
{
	if(!bat_adc_is_runing)
	{
		startBatteryMeasurement();
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

	Logger(LOG_INF, "Battery: %lu mV, %u%%, (%u)", batteryVoltage, batteryLevel, system_powerStatus());

	return batteryLevel;
}


static void startBatteryMeasurement(void)
{
	HAL_GPIO_WritePin(BAT_ADC_GND_GPIO_Port, BAT_ADC_GND_Pin, GPIO_PIN_RESET);
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*) batteryAdcData, BAT_VOLTAGE_SAMPLES*2);
	bat_adc_is_runing = 1;
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

//battery voltage measurement callback
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
