/*
 * forecast.c
 *
 *  Created on: 13.04.2022
 *      Author: Karol
 */

#include "forecast.h"
#include "logger.h"
#include "fatfs.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define PARSER_BUFF_SIZE 1024

static uint8_t findInFile(FIL *file, char* str, char *buffer, uint32_t bufferSize);
static char* getEndOfJsonObj(char* const jsonObjStartPtr);
static char* getForecastObjJson(char* jsonData);
static char* getJsonValuePtr(char* key, char* jsonObjStr);
static uint8_t getDecymalValue(char* dataPtr);
static uint8_t windSpeedToBofort(uint16_t windSpeed);
static EWeaterCondition codeToCondition(uint16_t weatherId);
static uint16_t parseRainSnowValue(char* jsonSubObject);


/* PUBLIC FUNCTIONS */

uint8_t parseForecast(char * jsonFileName, SForecast *forecastData)
{
	char parseBuffer[PARSER_BUFF_SIZE];
	FIL forecastFile;
	FRESULT f_status;
	UINT bytesRead = 0;
	char *dataPtr = parseBuffer;
	bool parsingOK = true;
	uint8_t forecastObjCnt;

	f_status = f_open(&forecastFile, jsonFileName, FA_READ);
	if(f_status != FR_OK)
	{
		Logger(LOG_ERR, "Open file %s. Err %d", jsonFileName, f_status);
		return 1;
	}

	//parse timezone offset
	if(0 != findInFile(&forecastFile, "\"timezone_offset\"", parseBuffer, PARSER_BUFF_SIZE))
	{
		Logger(LOG_ERR, "Parser err. \"timezone_offset\" not found");
		f_close(&forecastFile);
		return 2;
	}

	f_read(&forecastFile, parseBuffer, 128, &bytesRead);
	parseBuffer[bytesRead-1] = '\0';
	dataPtr = strchr(parseBuffer, ':');
	if(dataPtr == NULL)
	{
		Logger(LOG_ERR, "Parser err. timezone_offset value err");
		f_close(&forecastFile);
		return 2;
	}
	forecastData->timeOffset = atoi(dataPtr+1);

	//start parsing forecast
	if(0 != findInFile(&forecastFile, "\"hourly\"", parseBuffer, PARSER_BUFF_SIZE))
	{
		Logger(LOG_ERR, "Parser err. hourly not found");
		f_close(&forecastFile);
		return 2;
	}

	for(forecastObjCnt = 0; forecastObjCnt < FORECAST_SIZE; forecastObjCnt++)
	{
		char* jsonObjStr;

		f_read(&forecastFile, parseBuffer, PARSER_BUFF_SIZE, &bytesRead);

		parseBuffer[bytesRead-1] = '\0';
		jsonObjStr = getForecastObjJson(parseBuffer);
		if(jsonObjStr == NULL)
		{
			Logger(LOG_ERR, "Parser err. Parsed %d obj.", forecastObjCnt);
			return 2;
		}

		//move file pointer to next json object
		f_lseek(&forecastFile, (f_tell(&forecastFile) - bytesRead + ((jsonObjStr - parseBuffer) + strlen(jsonObjStr))));

		//parsing time
		dataPtr = getJsonValuePtr("\"dt\"", jsonObjStr);
		if(dataPtr != NULL)
		{
			forecastData->hourForecast[forecastObjCnt].time = (time_t)atoi(dataPtr);
		}
		else
		{
			parsingOK = false;
			break;
		}

		//parsing temp
		dataPtr = getJsonValuePtr("\"temp\"", jsonObjStr);
		if(dataPtr != NULL)
		{
			forecastData->hourForecast[forecastObjCnt].temp = (int16_t)atoi(dataPtr) * 10;
			forecastData->hourForecast[forecastObjCnt].temp += getDecymalValue(dataPtr);
		}
		else
		{
			parsingOK = false;
			break;
		}

		//parsing temp feel
		dataPtr = getJsonValuePtr("\"feels_like\"", jsonObjStr);
		if(dataPtr != NULL)
		{
			forecastData->hourForecast[forecastObjCnt].temp_feel = (int16_t)atoi(dataPtr) * 10;
			forecastData->hourForecast[forecastObjCnt].temp_feel += getDecymalValue(dataPtr);
		}
		else
		{
			parsingOK = false;
			break;
		}

		//parsing pressure
		dataPtr = getJsonValuePtr("\"pressure\"", jsonObjStr);
		if(dataPtr != NULL)
		{
			forecastData->hourForecast[forecastObjCnt].pressure = (uint16_t)atoi(dataPtr);
		}
		else
		{
			parsingOK = false;
			break;
		}

		//parsing wind speed
		dataPtr = getJsonValuePtr("\"wind_speed\"", jsonObjStr);
		if(dataPtr != NULL)
		{
			uint16_t windSpeed = (int16_t)atoi(dataPtr) * 10;
			windSpeed += getDecymalValue(dataPtr);
			forecastData->hourForecast[forecastObjCnt].wind = windSpeedToBofort(windSpeed);
		}
		else
		{
			parsingOK = false;
			break;
		}

		//parsing wind speed gust
		forecastData->hourForecast[forecastObjCnt].wind_gust = forecastData->hourForecast[forecastObjCnt].wind;
		dataPtr = getJsonValuePtr("\"wind_gust\"", jsonObjStr);
		if(dataPtr != NULL)
		{
			uint16_t windSpeed = (uint16_t)atoi(dataPtr) * 10;
			windSpeed += getDecymalValue(dataPtr);
			forecastData->hourForecast[forecastObjCnt].wind_gust = windSpeedToBofort(windSpeed);
		}

		//weather id
		dataPtr = getJsonValuePtr("\"id\"", jsonObjStr);
		if(dataPtr != NULL)
		{
			forecastData->hourForecast[forecastObjCnt].condition = codeToCondition(atoi(dataPtr));
		}
		else
		{
			parsingOK = false;
			break;
		}

		//rain
		forecastData->hourForecast[forecastObjCnt].rain = 0;
		dataPtr = getJsonValuePtr("\"rain\"", jsonObjStr);
		if(dataPtr != NULL)
		{
			forecastData->hourForecast[forecastObjCnt].rain = parseRainSnowValue(dataPtr);
		}

		//rain
		forecastData->hourForecast[forecastObjCnt].snow = 0;
		dataPtr = getJsonValuePtr("\"snow\"", jsonObjStr);
		if(dataPtr != NULL)
		{
			forecastData->hourForecast[forecastObjCnt].snow = parseRainSnowValue(dataPtr);
		}

		//Probability of precipitation
		dataPtr = getJsonValuePtr("\"pop\"", jsonObjStr);
		if(dataPtr != NULL)
		{
			forecastData->hourForecast[forecastObjCnt].pop = (uint8_t)atoi(dataPtr) * 100;
			forecastData->hourForecast[forecastObjCnt].pop += getDecymalValue(dataPtr)*10;
		}
		else
		{
			parsingOK = false;
			break;
		}

	}

	if(!parsingOK)
	{
		Logger(LOG_ERR, "Parser err. Parsed %d obj.", forecastObjCnt);
		return 2;
	}

	f_close(&forecastFile);

	Logger(LOG_INF, "Parsed %d obj.", forecastObjCnt);
	return 0;
}




/* PRIVATE FUNCTIONS */

static uint8_t findInFile(FIL *file, char* str, char *buffer, uint32_t bufferSize)
{
	const uint32_t readOverlap = strlen(str);
	UINT bytesRead = 0;
	char *dataPtr = NULL;

	f_rewind(file);
	if(FR_OK != f_read(file, buffer, bufferSize, &bytesRead)) { return 2; }

	while(NULL == (dataPtr = strnstr(buffer, str, bytesRead)))
	{
		if(f_size(file) == f_tell(file)) { return 1; }
		if(FR_OK != f_lseek(file, f_tell(file) - readOverlap)) { return 2; }
		if(FR_OK != f_read(file, buffer, PARSER_BUFF_SIZE, &bytesRead)) { return 2; }
	}

	if(FR_OK != f_lseek(file, f_tell(file) - bytesRead + (dataPtr - buffer))) { return 2; }

	return 0;
}

/**
 * Looks for end of pointed json object
 * @param jsonObjStartPtr start of json object. String have to ended with \0 sign.
 * @return pointer to end of json object. NULL if json object is corrupted.
 */
static char* getEndOfJsonObj(char* const jsonObjStartPtr)
{
	char* endObjectPtr = NULL;
	char* dataPtr = jsonObjStartPtr;
	int8_t stack = 1;

	if(*jsonObjStartPtr != '{')
	{
		return NULL;
	}

	while(1)
	{
		dataPtr++;

		if(*(dataPtr) == '\0')
		{
			break;
		}
		else if(*(dataPtr) == '{')
		{
			stack++;
		}
		else if(*(dataPtr) == '}')
		{
			stack--;
			if(stack == 0)
			{
				endObjectPtr = dataPtr+1;
				break;
			}
			else if(stack < 0)
			{
				break;
			}
		}
	}

	return endObjectPtr;
}

static char* getForecastObjJson(char* jsonData)
{
	char* dataPtr;
	char* endObjectPtr;

	dataPtr = strchr(jsonData, '{'); //looking for start of object
	if(dataPtr == NULL)
	{
		return NULL;
	}

	endObjectPtr = getEndOfJsonObj(dataPtr); //looking for end of object
	if(endObjectPtr == NULL)
	{
		return NULL;
	}

	*endObjectPtr = '\0'; //terminate end of object
	return dataPtr;
}

static char* getJsonValuePtr(char* key, char* jsonObjStr)
{
	char* dataPtr;
	dataPtr = strstr(jsonObjStr, key);
	if(dataPtr == NULL) return NULL;

	dataPtr = strchr(dataPtr, ':');
	if(dataPtr == NULL) return NULL;

	return dataPtr+1;
}

static uint8_t getDecymalValue(char* dataPtr)
{
	for(uint8_t i = 0 ; ; i++)
	{
		if(*(dataPtr+i) == ',' || *(dataPtr+i) == '\0')
		{
			break;
		}
		else if(*(dataPtr+i) == '.')
		{
			char candidate = *(dataPtr+i+1);
			if(candidate > 47 && candidate < 58)
			{
				return candidate-48;
			}
			else
			{
				break;
			}
		}
	}

	return 0;
}

static uint8_t windSpeedToBofort(uint16_t windSpeed)
{
	if(windSpeed < 2)    return 0;
	if(windSpeed <= 15)	 return 1;
	if(windSpeed <= 33)	 return 2;
	if(windSpeed <= 54)  return 3;
	if(windSpeed <= 79)  return 4;
	if(windSpeed <= 107) return 5;
	if(windSpeed <= 138) return 6;
	if(windSpeed <= 171) return 7;
	if(windSpeed <= 207) return 8;
	if(windSpeed <= 244) return 9;
	if(windSpeed <= 284) return 10;
	if(windSpeed <= 326) return 11;
	return 12;
}

static EWeaterCondition codeToCondition(uint16_t weatherId)
{
	if (2 == (weatherId/100)) return WEATER_STORM;
	if (3 == (weatherId/100)) return WEATER_DRIZZLE;
	if (5 == (weatherId/100)) return WEATER_RAIN;
	if (6 == (weatherId/100)) return WEATER_SNOW;
	if (7 == (weatherId/100)) return WEATER_ATMOSPHERE;
	if (800 == (weatherId))   return WEATER_CLEAR;
	else return WEATER_CLOUDS;
}

static uint16_t parseRainSnowValue(char* jsonSubObject)
{
	char* dataPtr;
	char* endSubObj;
	uint16_t objLen = 0;
	uint16_t value;

	if(*jsonSubObject != '{') return 0;

	endSubObj = strchr(jsonSubObject, '}');
	if(endSubObj == NULL) return 0;

	objLen = endSubObj - jsonSubObject + 1;
	dataPtr = strnstr(jsonSubObject, "\"1h\"", objLen);
	if(dataPtr == NULL) return 0;
	dataPtr = strchr(dataPtr, ':');
	if(dataPtr == NULL) return 0;
	dataPtr++;
	value = (uint16_t)atoi(dataPtr) * 10;
	value += getDecymalValue(dataPtr);
	return value;
}

