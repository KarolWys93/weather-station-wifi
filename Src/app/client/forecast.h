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

#define FORECAST_SIZE 48

typedef enum EWeaterCondition
{
	WEATER_STORM,
	WEATER_DRIZZLE,
	WEATER_RAIN,
	WEATER_SNOW,
	WEATER_ATMOSPHERE,
	WEATER_CLEAR,
	WEATER_CLOUDS
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
	time_t timeOffset;
	SForecastHour hourForecast[FORECAST_SIZE];
} SForecast;


uint8_t parseForecast(char * jsonFileName, SForecast *forecastData);


#endif /* APP_CLIENT_FORECAST_H_ */
