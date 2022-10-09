/*
 * Forecast.h
 *
 *  Created on: 13.04.2022
 *      Author: Karol
 */

#ifndef APP_CLIENT_FORECAST_H_
#define APP_CLIENT_FORECAST_H_

#include <stdint.h>
#include <time.h>

//#define FORECAST_SIZE 48
#define FORECAST_SIZE 72

typedef enum EWeaterCondition
{
	WEATHER_STORM,
	WEATHER_DRIZZLE,
	WEATHER_RAIN,
	WEATHER_SNOW,
	WEATHER_ATMOSPHERE,
	WEATHER_CLEAR,
	WEATHER_CLOUDS
} EWeaterCondition;

typedef struct SForecastHour
{
	time_t time;
	int16_t temp;
	int16_t temp_feel;
	uint16_t pressure;
	uint8_t wind;
	uint8_t wind_gust;
	uint16_t rain;
	uint16_t snow;
	uint8_t pop;
	EWeaterCondition condition;
} SForecastHour;

typedef struct SForecast
{
	time_t timeZoneOffset;
	SForecastHour hourForecast[FORECAST_SIZE];
} SForecast;


uint8_t parseForecast(char * jsonFileName, SForecast *forecastData);
uint8_t parseForecast2(char * jsonFileName, SForecast *forecastData);

#endif /* APP_CLIENT_FORECAST_H_ */
