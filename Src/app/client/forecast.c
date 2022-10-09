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
static uint8_t findInFileSeek(FIL *file, char* str, char *buffer, uint32_t bufferSize, uint32_t seek);
static char* getJsonArrayPtr(FIL *file, const uint32_t fileOffset, char* str, char *buffer, const uint32_t bufferSize);
//static char* getEndOfJsonObj(char* const jsonObjStartPtr);
//static char* getForecastObjJson(char* jsonData);
static char* getJsonValuePtr(char* key, char* jsonObjStr);
static uint8_t getDecymalValue(char* dataPtr);
static char* getNextJsonArrayValue(char* dataPtr);
//static uint8_t windSpeedToBeaufort(uint16_t windSpeed);
static uint8_t windSpeedToBeaufort2(uint16_t windSpeed);
//static EWeaterCondition codeToCondition(uint16_t weatherId);
static EWeaterCondition codeToCondition2(uint16_t weatherId);
//static uint16_t parseRainSnowValue(char* jsonSubObject);


/* PUBLIC FUNCTIONS */
uint8_t parseForecast2(char * jsonFileName, SForecast *forecastData)
{
	char parseBuffer[PARSER_BUFF_SIZE];
	FIL forecastFile;
	FRESULT f_status;
	UINT bytesRead = 0;
	char *dataPtr = parseBuffer;
	uint8_t forecastObjCnt;

	uint32_t startOfJsonFileOffset = 0;


	//test
	memset(forecastData, 0, sizeof(SForecast));

	f_status = f_open(&forecastFile, jsonFileName, FA_READ);
	if(f_status != FR_OK)
	{
		Logger(LOG_ERR, "Open file %s. Err %d", jsonFileName, f_status);
		return 1;
	}

	//parse timezone offset
	if(0 != findInFile(&forecastFile, "\"utc_offset_seconds\"", parseBuffer, PARSER_BUFF_SIZE))
	{
		Logger(LOG_ERR, "Parser err. \"utc_offset_seconds\" not found");
		f_close(&forecastFile);
		return 2;
	}

	f_read(&forecastFile, parseBuffer, 128, &bytesRead);
	parseBuffer[bytesRead-1] = '\0';
	dataPtr = getJsonValuePtr("utc_offset_seconds", parseBuffer);
	if(dataPtr == NULL)
	{
		Logger(LOG_ERR, "Parser err. utc_offset_seconds value err");
		f_close(&forecastFile);
		return 2;
	}
	forecastData->timeZoneOffset = atoi(dataPtr);


	//start parsing forecast
	if(0 != findInFile(&forecastFile, "\"hourly\"", parseBuffer, PARSER_BUFF_SIZE))
	{
		Logger(LOG_ERR, "Parser err. hourly not found");
		f_close(&forecastFile);
		return 2;
	}

	startOfJsonFileOffset = f_tell(&forecastFile);

	/* time */
	dataPtr = getJsonArrayPtr(&forecastFile, startOfJsonFileOffset, "\"time\"", parseBuffer, PARSER_BUFF_SIZE);

	if(NULL == dataPtr) return 2;
	dataPtr++;

	for(forecastObjCnt = 0; forecastObjCnt < FORECAST_SIZE; forecastObjCnt++)
	{
		forecastData->hourForecast[forecastObjCnt].time = (time_t)atoi(dataPtr);
		dataPtr = getNextJsonArrayValue(dataPtr);
		if(dataPtr == NULL && ((forecastObjCnt + 1) != FORECAST_SIZE))
		{
			return 2; //array end to early
		}
	}

	/* temperature_2m */
	dataPtr = getJsonArrayPtr(&forecastFile, startOfJsonFileOffset, "\"temperature_2m\"", parseBuffer, PARSER_BUFF_SIZE);
	if(NULL == dataPtr) return 2;
	dataPtr++;

	for(forecastObjCnt = 0; forecastObjCnt < FORECAST_SIZE; forecastObjCnt++)
	{
		forecastData->hourForecast[forecastObjCnt].temp = (int16_t)atoi(dataPtr) * 10;
		forecastData->hourForecast[forecastObjCnt].temp += getDecymalValue(dataPtr);
		dataPtr = getNextJsonArrayValue(dataPtr);
		if(dataPtr == NULL && ((forecastObjCnt + 1) != FORECAST_SIZE))
		{
			return 2; //array end to early
		}
	}

	/* apparent_temperature */
	dataPtr = getJsonArrayPtr(&forecastFile, startOfJsonFileOffset, "\"apparent_temperature\"", parseBuffer, PARSER_BUFF_SIZE);
	if(NULL == dataPtr) return 2;
	dataPtr++;

	for(forecastObjCnt = 0; forecastObjCnt < FORECAST_SIZE; forecastObjCnt++)
	{
		forecastData->hourForecast[forecastObjCnt].temp_feel = (int16_t)atoi(dataPtr) * 10;
		forecastData->hourForecast[forecastObjCnt].temp_feel += getDecymalValue(dataPtr);
		dataPtr = getNextJsonArrayValue(dataPtr);
		if(dataPtr == NULL && ((forecastObjCnt + 1) != FORECAST_SIZE))
		{
			return 2; //array end to early
		}
	}

	/* rain */
	dataPtr = getJsonArrayPtr(&forecastFile, startOfJsonFileOffset, "\"rain\"", parseBuffer, PARSER_BUFF_SIZE);
	if(NULL == dataPtr) return 2;
	dataPtr++;

	for(forecastObjCnt = 0; forecastObjCnt < FORECAST_SIZE; forecastObjCnt++)
	{
		forecastData->hourForecast[forecastObjCnt].rain = (int16_t)atoi(dataPtr) * 10;
		forecastData->hourForecast[forecastObjCnt].rain += getDecymalValue(dataPtr);
		dataPtr = getNextJsonArrayValue(dataPtr);
		if(dataPtr == NULL && ((forecastObjCnt + 1) != FORECAST_SIZE))
		{
			return 2; //array end to early
		}
	}

	/* showers - convective rain */
	dataPtr = getJsonArrayPtr(&forecastFile, startOfJsonFileOffset, "\"showers\"", parseBuffer, PARSER_BUFF_SIZE);
	if(NULL == dataPtr) return 2;
	dataPtr++;

	for(forecastObjCnt = 0; forecastObjCnt < FORECAST_SIZE; forecastObjCnt++)
	{
		forecastData->hourForecast[forecastObjCnt].rain += (int16_t)atoi(dataPtr) * 10;
		forecastData->hourForecast[forecastObjCnt].rain += getDecymalValue(dataPtr);
		dataPtr = getNextJsonArrayValue(dataPtr);
		if(dataPtr == NULL && ((forecastObjCnt + 1) != FORECAST_SIZE))
		{
			return 2; //array end to early
		}
	}

	/* snowfall*/
	dataPtr = getJsonArrayPtr(&forecastFile, startOfJsonFileOffset, "\"snowfall\"", parseBuffer, PARSER_BUFF_SIZE);
	if(NULL == dataPtr) return 2;
	dataPtr++;

	for(forecastObjCnt = 0; forecastObjCnt < FORECAST_SIZE; forecastObjCnt++)
	{
		forecastData->hourForecast[forecastObjCnt].snow = (int16_t)atoi(dataPtr) * 10;
		forecastData->hourForecast[forecastObjCnt].snow += getDecymalValue(dataPtr);
		dataPtr = getNextJsonArrayValue(dataPtr);
		if(dataPtr == NULL && ((forecastObjCnt + 1) != FORECAST_SIZE))
		{
			return 2; //array end to early
		}
	}

	/* weathercode*/
	dataPtr = getJsonArrayPtr(&forecastFile, startOfJsonFileOffset, "\"weathercode\"", parseBuffer, PARSER_BUFF_SIZE);
	if(NULL == dataPtr) return 2;
	dataPtr++;

	for(forecastObjCnt = 0; forecastObjCnt < FORECAST_SIZE; forecastObjCnt++)
	{
		forecastData->hourForecast[forecastObjCnt].condition = codeToCondition2(atoi(dataPtr));
		dataPtr = getNextJsonArrayValue(dataPtr);
		if(dataPtr == NULL && ((forecastObjCnt + 1) != FORECAST_SIZE))
		{
			return 2; //array end to early
		}
	}

	/* pressure_msl*/
	dataPtr = getJsonArrayPtr(&forecastFile, startOfJsonFileOffset, "\"pressure_msl\"", parseBuffer, PARSER_BUFF_SIZE);
	if(NULL == dataPtr) return 2;
	dataPtr++;

	for(forecastObjCnt = 0; forecastObjCnt < FORECAST_SIZE; forecastObjCnt++)
	{
		forecastData->hourForecast[forecastObjCnt].pressure = (uint16_t)atoi(dataPtr);
		dataPtr = getNextJsonArrayValue(dataPtr);
		if(dataPtr == NULL && ((forecastObjCnt + 1) != FORECAST_SIZE))
		{
			return 2; //array end to early
		}
	}

	/* windspeed_10m*/
	dataPtr = getJsonArrayPtr(&forecastFile, startOfJsonFileOffset, "\"windspeed_10m\"", parseBuffer, PARSER_BUFF_SIZE);
	if(NULL == dataPtr) return 2;
	dataPtr++;

	for(forecastObjCnt = 0; forecastObjCnt < FORECAST_SIZE; forecastObjCnt++)
	{
		uint16_t windSpeed = (int16_t)atoi(dataPtr);
		forecastData->hourForecast[forecastObjCnt].wind = windSpeedToBeaufort2(windSpeed);
		dataPtr = getNextJsonArrayValue(dataPtr);
		if(dataPtr == NULL && ((forecastObjCnt + 1) != FORECAST_SIZE))
		{
			return 2; //array end to early
		}
	}

	/* windgusts_10m*/
	dataPtr = getJsonArrayPtr(&forecastFile, startOfJsonFileOffset, "\"windgusts_10m\"", parseBuffer, PARSER_BUFF_SIZE);
	if(NULL == dataPtr) return 2;
	dataPtr++;

	for(forecastObjCnt = 0; forecastObjCnt < FORECAST_SIZE; forecastObjCnt++)
	{
		uint16_t windSpeed = (uint16_t)atoi(dataPtr);
		forecastData->hourForecast[forecastObjCnt].wind_gust = windSpeedToBeaufort2(windSpeed);
		dataPtr = getNextJsonArrayValue(dataPtr);
		if(dataPtr == NULL && ((forecastObjCnt + 1) != FORECAST_SIZE))
		{
			return 2; //array end to early
		}
	}

	f_close(&forecastFile);

	Logger(LOG_INF, "Parsed OK");
	return 0;
}

//uint8_t parseForecast(char * jsonFileName, SForecast *forecastData)
//{
//	char parseBuffer[PARSER_BUFF_SIZE];
//	FIL forecastFile;
//	FRESULT f_status;
//	UINT bytesRead = 0;
//	char *dataPtr = parseBuffer;
//	bool parsingOK = true;
//	uint8_t forecastObjCnt;
//
//	//test
//	memset(forecastData, 0, sizeof(SForecast));
//
//	f_status = f_open(&forecastFile, jsonFileName, FA_READ);
//	if(f_status != FR_OK)
//	{
//		Logger(LOG_ERR, "Open file %s. Err %d", jsonFileName, f_status);
//		return 1;
//	}
//
//	//parse timezone offset
//	if(0 != findInFile(&forecastFile, "\"timezone_offset\"", parseBuffer, PARSER_BUFF_SIZE))
//	{
//		Logger(LOG_ERR, "Parser err. \"timezone_offset\" not found");
//		f_close(&forecastFile);
//		return 2;
//	}
//
//	f_read(&forecastFile, parseBuffer, 128, &bytesRead);
//	parseBuffer[bytesRead-1] = '\0';
//	dataPtr = strchr(parseBuffer, ':');
//	if(dataPtr == NULL)
//	{
//		Logger(LOG_ERR, "Parser err. timezone_offset value err");
//		f_close(&forecastFile);
//		return 2;
//	}
//	forecastData->timeZoneOffset = atoi(dataPtr+1);
//
//	//start parsing forecast
//	if(0 != findInFile(&forecastFile, "\"hourly\"", parseBuffer, PARSER_BUFF_SIZE))
//	{
//		Logger(LOG_ERR, "Parser err. hourly not found");
//		f_close(&forecastFile);
//		return 2;
//	}
//
//	for(forecastObjCnt = 0; forecastObjCnt < 48; forecastObjCnt++)
//	{
//		char* jsonObjStr;
//
//		f_read(&forecastFile, parseBuffer, PARSER_BUFF_SIZE, &bytesRead);
//
//		parseBuffer[bytesRead-1] = '\0';
//		jsonObjStr = getForecastObjJson(parseBuffer);
//		if(jsonObjStr == NULL)
//		{
//			Logger(LOG_ERR, "Parser err. Parsed %d obj.", forecastObjCnt);
//			return 2;
//		}
//
//		//move file pointer to next json object
//		f_lseek(&forecastFile, (f_tell(&forecastFile) - bytesRead + ((jsonObjStr - parseBuffer) + strlen(jsonObjStr))));
//
//		//parsing time
//		dataPtr = getJsonValuePtr("\"dt\"", jsonObjStr);
//		if(dataPtr != NULL)
//		{
//			forecastData->hourForecast[forecastObjCnt].time = (time_t)atoi(dataPtr);
//		}
//		else
//		{
//			parsingOK = false;
//			break;
//		}
//
//		//parsing temp
//		dataPtr = getJsonValuePtr("\"temp\"", jsonObjStr);
//		if(dataPtr != NULL)
//		{
//			forecastData->hourForecast[forecastObjCnt].temp = (int16_t)atoi(dataPtr) * 10;
//			forecastData->hourForecast[forecastObjCnt].temp += getDecymalValue(dataPtr);
//		}
//		else
//		{
//			parsingOK = false;
//			break;
//		}
//
//		//parsing temp feel
//		dataPtr = getJsonValuePtr("\"feels_like\"", jsonObjStr);
//		if(dataPtr != NULL)
//		{
//			forecastData->hourForecast[forecastObjCnt].temp_feel = (int16_t)atoi(dataPtr) * 10;
//			forecastData->hourForecast[forecastObjCnt].temp_feel += getDecymalValue(dataPtr);
//		}
//		else
//		{
//			parsingOK = false;
//			break;
//		}
//
//		//parsing pressure
//		dataPtr = getJsonValuePtr("\"pressure\"", jsonObjStr);
//		if(dataPtr != NULL)
//		{
//			forecastData->hourForecast[forecastObjCnt].pressure = (uint16_t)atoi(dataPtr);
//		}
//		else
//		{
//			parsingOK = false;
//			break;
//		}
//
//		//parsing wind speed
//		dataPtr = getJsonValuePtr("\"wind_speed\"", jsonObjStr);
//		if(dataPtr != NULL)
//		{
//			uint16_t windSpeed = (int16_t)atoi(dataPtr) * 10;
//			windSpeed += getDecymalValue(dataPtr);
//			forecastData->hourForecast[forecastObjCnt].wind = windSpeedToBeaufort(windSpeed);
//		}
//		else
//		{
//			parsingOK = false;
//			break;
//		}
//
//		//parsing wind speed gust
//		forecastData->hourForecast[forecastObjCnt].wind_gust = forecastData->hourForecast[forecastObjCnt].wind;
//		dataPtr = getJsonValuePtr("\"wind_gust\"", jsonObjStr);
//		if(dataPtr != NULL)
//		{
//			uint16_t windSpeed = (uint16_t)atoi(dataPtr) * 10;
//			windSpeed += getDecymalValue(dataPtr);
//			forecastData->hourForecast[forecastObjCnt].wind_gust = windSpeedToBeaufort(windSpeed);
//		}
//
//		//weather id
//		dataPtr = getJsonValuePtr("\"id\"", jsonObjStr);
//		if(dataPtr != NULL)
//		{
//			forecastData->hourForecast[forecastObjCnt].condition = codeToCondition(atoi(dataPtr));
//		}
//		else
//		{
//			parsingOK = false;
//			break;
//		}
//
//		//rain
//		forecastData->hourForecast[forecastObjCnt].rain = 0;
//		dataPtr = getJsonValuePtr("\"rain\"", jsonObjStr);
//		if(dataPtr != NULL)
//		{
//			forecastData->hourForecast[forecastObjCnt].rain = parseRainSnowValue(dataPtr);
//		}
//
//		//rain
//		forecastData->hourForecast[forecastObjCnt].snow = 0;
//		dataPtr = getJsonValuePtr("\"snow\"", jsonObjStr);
//		if(dataPtr != NULL)
//		{
//			forecastData->hourForecast[forecastObjCnt].snow = parseRainSnowValue(dataPtr);
//		}
//
//		//Probability of precipitation
//		dataPtr = getJsonValuePtr("\"pop\"", jsonObjStr);
//		if(dataPtr != NULL)
//		{
//			forecastData->hourForecast[forecastObjCnt].pop = (uint8_t)atoi(dataPtr) * 100;
//			forecastData->hourForecast[forecastObjCnt].pop += getDecymalValue(dataPtr)*10;
//		}
//		else
//		{
//			parsingOK = false;
//			break;
//		}
//
//	}
//
//	if(!parsingOK)
//	{
//		Logger(LOG_ERR, "Parser err. Parsed %d obj.", forecastObjCnt);
//		return 2;
//	}
//
//	f_close(&forecastFile);
//
//	Logger(LOG_INF, "Parsed %d obj.", forecastObjCnt);
//	return 0;
//}



/* PRIVATE FUNCTIONS */

static uint8_t findInFileSeek(FIL *file, char* str, char *buffer, uint32_t bufferSize, uint32_t seek)
{
	const uint32_t readOverlap = strlen(str);
	UINT bytesRead = 0;
	char *dataPtr = NULL;

	f_lseek(file, seek);
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

static uint8_t findInFile(FIL *file, char* str, char *buffer, uint32_t bufferSize)
{
	return findInFileSeek(file, str, buffer, bufferSize, 0);
}

static char* getJsonArrayPtr(FIL *file, const uint32_t fileOffset, char* str, char *buffer, const uint32_t bufferSize)
{

	char * dataPtr = NULL;
	UINT bytesRead = 0;

	if(0 != findInFileSeek(file, str, buffer, PARSER_BUFF_SIZE, fileOffset))
	{
		Logger(LOG_ERR, "Parser err. %s not found", str);
		return NULL;
	}

	f_read(file, buffer, PARSER_BUFF_SIZE, &bytesRead);
	buffer[bytesRead-1] = '\0';
	dataPtr = getJsonValuePtr(str, buffer);
	if(NULL == dataPtr)
	{
		Logger(LOG_ERR, "Value error. %s", str);
	}

	return dataPtr;
}

/**
 * Looks for end of pointed json object
 * @param jsonObjStartPtr start of json object. String have to ended with \0 sign.
 * @return pointer to end of json object. NULL if json object is corrupted.
 */
//static char* getEndOfJsonObj(char* const jsonObjStartPtr)
//{
//	char* endObjectPtr = NULL;
//	char* dataPtr = jsonObjStartPtr;
//	int8_t stack = 1;
//
//	if(*jsonObjStartPtr != '{')
//	{
//		return NULL;
//	}
//
//	while(1)
//	{
//		dataPtr++;
//
//		if(*(dataPtr) == '\0')
//		{
//			break;
//		}
//		else if(*(dataPtr) == '{')
//		{
//			stack++;
//		}
//		else if(*(dataPtr) == '}')
//		{
//			stack--;
//			if(stack == 0)
//			{
//				endObjectPtr = dataPtr+1;
//				break;
//			}
//			else if(stack < 0)
//			{
//				break;
//			}
//		}
//	}
//
//	return endObjectPtr;
//}

//static char* getForecastObjJson(char* jsonData)
//{
//	char* dataPtr;
//	char* endObjectPtr;
//
//	dataPtr = strchr(jsonData, '{'); //looking for start of object
//	if(dataPtr == NULL)
//	{
//		return NULL;
//	}
//
//	endObjectPtr = getEndOfJsonObj(dataPtr); //looking for end of object
//	if(endObjectPtr == NULL)
//	{
//		return NULL;
//	}
//
//	*endObjectPtr = '\0'; //terminate end of object
//	return dataPtr;
//}

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

static char* getNextJsonArrayValue(char* dataPtr)
{
	while(1)
	{
		char dataChar = *dataPtr;
		if(dataChar == ',')
		{
			return (dataPtr + 1);
		}
		else if(dataChar == ']' || dataChar == '\0')
		{
			return NULL;
		}
		else
		{
			dataPtr++;
		}
	}
}

//static uint8_t windSpeedToBeaufort(uint16_t windSpeed)
//{
//	if(windSpeed < 2)    return 0;
//	if(windSpeed <= 15)	 return 1;
//	if(windSpeed <= 33)	 return 2;
//	if(windSpeed <= 54)  return 3;
//	if(windSpeed <= 79)  return 4;
//	if(windSpeed <= 107) return 5;
//	if(windSpeed <= 138) return 6;
//	if(windSpeed <= 171) return 7;
//	if(windSpeed <= 207) return 8;
//	if(windSpeed <= 244) return 9;
//	if(windSpeed <= 284) return 10;
//	if(windSpeed <= 326) return 11;
//	return 12;
//}

static uint8_t windSpeedToBeaufort2(uint16_t windSpeed)
{
	if(windSpeed == 0)    return 0;
	if(windSpeed <= 6)	 return 1;
	if(windSpeed <= 11)	 return 2;
	if(windSpeed <= 19)  return 3;
	if(windSpeed <= 29)  return 4;
	if(windSpeed <= 39) return 5;
	if(windSpeed <= 50) return 6;
	if(windSpeed <= 62) return 7;
	if(windSpeed <= 75) return 8;
	if(windSpeed <= 87) return 9;
	if(windSpeed <= 102) return 10;
	if(windSpeed <= 117) return 11;
	return 12;
}

//static EWeaterCondition codeToCondition(uint16_t weatherId)
//{
//	if (2 == (weatherId/100)) return WEATHER_STORM;
//	if (3 == (weatherId/100)) return WEATHER_DRIZZLE;
//	if (5 == (weatherId/100)) return WEATHER_RAIN;
//	if (6 == (weatherId/100)) return WEATHER_SNOW;
//	if (7 == (weatherId/100)) return WEATHER_ATMOSPHERE;
//	if (800 == (weatherId))   return WEATHER_CLEAR;
//	else return WEATHER_CLOUDS;
//}


static EWeaterCondition codeToCondition2(uint16_t weatherId)
{
/*	0	Clear sky
	1, 2, 3	Mainly clear, partly cloudy, and overcast
	45, 48	Fog and depositing rime fog
	51, 53, 55	Drizzle: Light, moderate, and dense intensity
	56, 57	Freezing Drizzle: Light and dense intensity
	61, 63, 65	Rain: Slight, moderate and heavy intensity
	66, 67	Freezing Rain: Light and heavy intensity
	71, 73, 75	Snow fall: Slight, moderate, and heavy intensity
	77	Snow grains
	80, 81, 82	Rain showers: Slight, moderate, and violent
	85, 86	Snow showers slight and heavy
	95 *	Thunderstorm: Slight or moderate
	96, 99 *	Thunderstorm with slight and heavy hail
*/

	if(0 == weatherId) return WEATHER_CLEAR;
	if(4 == weatherId/10) return WEATHER_ATMOSPHERE;
	if(5 == weatherId/10) return WEATHER_DRIZZLE;
	if(6 == weatherId/10) return WEATHER_RAIN;
	if(7 == weatherId/10) return WEATHER_SNOW;
	if(weatherId >= 80 && weatherId <= 82) return WEATHER_RAIN;
	if(weatherId == 85 || weatherId == 86) return WEATHER_RAIN;
	if(9 == weatherId/10) return WEATHER_STORM;
	else return WEATHER_CLOUDS;
}

//static uint16_t parseRainSnowValue(char* jsonSubObject)
//{
//	char* dataPtr;
//	char* endSubObj;
//	uint16_t objLen = 0;
//	uint16_t value;
//
//	if(*jsonSubObject != '{') return 0;
//
//	endSubObj = strchr(jsonSubObject, '}');
//	if(endSubObj == NULL) return 0;
//
//	objLen = endSubObj - jsonSubObject + 1;
//	dataPtr = strnstr(jsonSubObject, "\"1h\"", objLen);
//	if(dataPtr == NULL) return 0;
//	dataPtr = strchr(dataPtr, ':');
//	if(dataPtr == NULL) return 0;
//	dataPtr++;
//	value = (uint16_t)atoi(dataPtr) * 10;
//	value += getDecymalValue(dataPtr);
//	return value;
//}

