/*
 * forecast_drawer.c
 *
 *  Created on: 01.05.2022
 *      Author: Karol
 */

#include "forecast_drawer.h"

#include "e-Paper/EPD_1in54b.h"
#include "Config/DEV_Config.h"
#include "GUI/GUI_Paint.h"
#include "e-Paper/ImageData.h"

#include "rtc.h"
#include <stdio.h>

#include "system.h"

#define DISPLAY_DATA_SIZE 48

typedef struct SRangesValues
{
	int16_t maxTemp;
	int16_t minTemp;
	int16_t maxFtemp;
	int16_t minFtemp;
	uint16_t maxRain;
	uint16_t maxSnow;
	uint16_t maxPressure;
	uint16_t minPressure;
	uint8_t maxWind;
	uint8_t maxGWind;
} SRangesValues;


//static void getRangeValues(SRangesValues *rangeValues, const SForecast * const forecastData);
static void getRangeValues2(SRangesValues *rangeValues, const SForecast * const forecastData, uint8_t offset);

void drawForecast2(const SForecast * const forecastData)
{
	SRangesValues ranges;
	uint8_t battery = system_batteryLevel();
	uint8_t isCharging = system_isCharging();

	uint8_t d_black[(EPD_WIDTH/8) * EPD_HEIGHT];
	uint8_t d_grey[(EPD_WIDTH/8) * EPD_HEIGHT];
	uint8_t *d_red = d_black;
	uint8_t startHour;

	char textBuff[64];
	time_t forecastTime;

	int16_t maxTempRange;
	int16_t minTempRange;
	int16_t avrTempLine;

	int16_t precipitationMax;

	uint8_t timeOffset = 0;
	time_t currentTime = RTC_getTime();
	currentTime -= currentTime%3600;

	for(uint8_t i = 0; i < FORECAST_SIZE; i++)
	{
		if(forecastData->hourForecast[i].time == currentTime)
		{
			timeOffset = i;
			break;
		}
	}

	forecastTime = forecastData->hourForecast[timeOffset].time + forecastData->timeZoneOffset;
	struct tm * timeObject = gmtime(&forecastTime);
	startHour = timeObject->tm_hour;

	getRangeValues2(&ranges, forecastData, timeOffset);

	//black & grey
	Paint_NewImage(d_black, EPD_WIDTH, EPD_HEIGHT, 0, WHITE);
	Paint_NewImage(d_grey, EPD_WIDTH, EPD_HEIGHT, 0, WHITE);

	Paint_SelectImage(d_black);
	Paint_Clear(WHITE);
	Paint_SelectImage(d_grey);
	Paint_Clear(WHITE);

	Paint_SelectImage(d_black);
	Paint_DrawBitMap(forecast_black);

	sprintf(textBuff, "%02d/%02d", timeObject->tm_mday, timeObject->tm_mon+1);
	Paint_DrawString(0, 8, textBuff, &Font8, WHITE, BLACK);

	for(uint8_t i = 0; i < 7; i++)
	{
		sprintf(textBuff, "%02d", startHour);
		Paint_DrawString(24*i + 31, 6, textBuff, &Font12, WHITE, BLACK);
		startHour = (startHour + 6) % 24;
	}

	//TEMPERATURE
	if((ranges.maxTemp%10))
		ranges.maxTemp = ranges.maxTemp + (10 - ranges.maxTemp%10);
	ranges.minTemp = 10*((ranges.minTemp/10));

	if((ranges.maxFtemp%10))
		ranges.maxFtemp = ranges.maxFtemp + (10 - ranges.maxFtemp%10);
	ranges.minFtemp = 10*((ranges.minFtemp/10));

	maxTempRange = ranges.maxTemp > ranges.maxFtemp ? ranges.maxTemp : ranges.maxFtemp;
	minTempRange = ranges.minTemp < ranges.minFtemp ? ranges.minTemp : ranges.minFtemp;


	sprintf(textBuff, "%3d", maxTempRange/10);
	Paint_DrawString(8, 21, textBuff, &Font12, WHITE, BLACK);
	sprintf(textBuff, "%3d", (maxTempRange + minTempRange)/20);
	Paint_DrawString(8, 41, textBuff, &Font12, WHITE, BLACK);
	sprintf(textBuff, "%3d", minTempRange/10);
	Paint_DrawString(8, 61, textBuff, &Font12, WHITE, BLACK);

	if(minTempRange < 0)
	{
		Paint_SelectImage(d_grey);
		Paint_DrawRectangle(31, (51*minTempRange)/(maxTempRange-minTempRange)+74, 199, 74, BLACK, DRAW_FILL_FULL, DOT_PIXEL_1X1);
		Paint_SelectImage(d_black);
	}

	avrTempLine = (-51*(((maxTempRange + minTempRange)/2)-minTempRange))/(maxTempRange-minTempRange) + 72;
	Paint_DrawLine(31, avrTempLine, 199, avrTempLine, BLACK, LINE_STYLE_DOTTED, DOT_PIXEL_1X1);

	for(uint8_t i = 1; i < 43; i++)
	{
		int16_t startTemp = forecastData->hourForecast[timeOffset + i - 1].temp;
		int16_t endTemp = forecastData->hourForecast[timeOffset + i].temp;

		uint16_t start = (-51*(startTemp-minTempRange))/(maxTempRange-minTempRange) + 72;
		uint16_t end = (-51*(endTemp-minTempRange))/(maxTempRange-minTempRange) + 72;

		Paint_DrawLine(4*(i-1)+31, start, 4*(i)+31, end, BLACK, LINE_STYLE_SOLID, DOT_PIXEL_2X2);
	}

	for(uint8_t i = 1; i < 43; i++)
	{
		int16_t startTemp = forecastData->hourForecast[timeOffset + i - 1].temp_feel;
		int16_t endTemp = forecastData->hourForecast[timeOffset + i].temp_feel;

		uint16_t start = (-51*(startTemp-minTempRange))/(maxTempRange-minTempRange) + 72;
		uint16_t end = (-51*(endTemp-minTempRange))/(maxTempRange-minTempRange) + 72;

		Paint_DrawLine(4*(i-1)+31, start, 4*(i)+31, end, BLACK, LINE_STYLE_SOLID, DOT_PIXEL_2X2);
	}

	//rain & snow
	ranges.maxRain = ranges.maxRain + (10 - ranges.maxRain%10);
	ranges.maxSnow = ranges.maxSnow + (10 - ranges.maxSnow%10);

	precipitationMax = ranges.maxRain > ranges.maxSnow ? ranges.maxRain : ranges.maxSnow;

	sprintf(textBuff, "%3d", precipitationMax/10);
	Paint_DrawString(8, 76, textBuff, &Font12, WHITE, BLACK);
	Paint_DrawString(8, 114, "  0", &Font12, WHITE, BLACK);

	for(uint8_t i = 0; i < 43; i++)
	{
		uint16_t rain = forecastData->hourForecast[timeOffset + i].rain;
		uint16_t snow = forecastData->hourForecast[timeOffset + i].snow;
		uint16_t startRain = (-51*rain)/(precipitationMax) + 125;
		uint16_t startSnow = (-51*snow)/(precipitationMax) + 125;

		if(rain != 0)
		{
			Paint_DrawRectangle(4*i+31, startRain, 4*i+32, 125, BLACK, DRAW_FILL_FULL, DOT_PIXEL_1X1);	//rain
		}
		if(snow != 0)
		{
			Paint_DrawRectangle(4*i+33, startSnow, 4*i+34, 125, BLACK, DRAW_FILL_FULL, DOT_PIXEL_1X1);	//snow
		}
		if(forecastData->hourForecast[timeOffset + i].condition == WEATHER_STORM)
		{
			Paint_DrawPoint(4*i+31, 75, WHITE, DOT_PIXEL_3X3, DOT_FILL_RIGHTUP);
			Paint_DrawPoint(4*i+31, 75, BLACK, DOT_PIXEL_3X3, DOT_FILL_RIGHTUP);
		}
	}

	//PRESSURE
	ranges.maxPressure = ranges.maxPressure + (5 - ranges.maxPressure%5);
	ranges.minPressure = ranges.minPressure - (ranges.minPressure%5);

	sprintf(textBuff, "%4d", ranges.maxPressure);
	Paint_DrawString(1, 126, textBuff, &Font12, WHITE, BLACK);
	sprintf(textBuff, "%4d", ranges.minPressure);
	Paint_DrawString(1, 165, textBuff, &Font12, WHITE, BLACK);

	for(uint8_t i = 0; i < 43; i++)
	{
		uint16_t pressure = forecastData->hourForecast[timeOffset + i].pressure;
		uint16_t start = (-51*(pressure-ranges.minPressure))/(ranges.maxPressure-ranges.minPressure) + 177;

		Paint_SelectImage(d_grey);
		Paint_DrawRectangle(4*(i)+31, start, 4*(i)+34, 177, BLACK, DRAW_FILL_FULL, DOT_PIXEL_1X1);

		Paint_SelectImage(d_black);
		Paint_DrawLine(4*(i)+31, start, 4*(i)+34, start, BLACK, LINE_STYLE_SOLID, DOT_PIXEL_1X1);	//rysowanie poza zakresem xend == 202
	}


	//wind
	if(ranges.maxGWind < 1) ranges.maxGWind = 1;
	sprintf(textBuff, "%2d", ranges.maxGWind);
	Paint_DrawString(15, 178, textBuff, &Font12, WHITE, BLACK);
	Paint_DrawString(15, 188, " 0", &Font12, WHITE, BLACK);

	for(uint8_t i = 0; i < 43; i++)
	{
		uint16_t windG = forecastData->hourForecast[timeOffset + i].wind_gust;
		uint16_t wind = forecastData->hourForecast[timeOffset + i].wind;

		uint16_t start = (-21*windG)/(ranges.maxGWind) + 200;
		uint16_t end = (-21*wind)/(ranges.maxGWind) + 200;

		Paint_SelectImage(d_black);
		Paint_DrawRectangle(4*(i)+31, start, 4*(i)+34, end, BLACK, DRAW_FILL_FULL, DOT_PIXEL_1X1);
		Paint_SelectImage(d_grey);
		Paint_DrawRectangle(4*(i)+31, end, 4*(i)+34, 200, BLACK, DRAW_FILL_FULL, DOT_PIXEL_1X1);

	}

	Paint_SelectImage(d_black);

	if(!isCharging)
	{
		Paint_DrawRectangle(0, 0, battery*2, 3, BLACK, DRAW_FILL_FULL, DOT_PIXEL_1X1);
	}
	else
	{
		Paint_DrawRectangle(0, 0, 199, 3, RED, DRAW_FILL_FULL, DOT_PIXEL_1X1);
	}

	EPD_SendBlackAndGrey(d_black, d_grey);

	//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

	//red
	Paint_SelectImage(d_red);
	Paint_Clear(WHITE);
	Paint_DrawBitMap(forecast_red);

	//feel temp
	for(uint8_t i = 1; i < 43; i++)
	{
		int16_t startTemp = forecastData->hourForecast[timeOffset + i-1].temp_feel;
		int16_t endTemp = forecastData->hourForecast[timeOffset + i].temp_feel;

		uint16_t start = (-51*(startTemp-minTempRange))/(maxTempRange-minTempRange) + 72;
		uint16_t end = (-51*(endTemp-minTempRange))/(maxTempRange-minTempRange) + 72;

		Paint_DrawLine(4*(i-1)+31, start, 4*(i)+31, end, RED, LINE_STYLE_SOLID, DOT_PIXEL_2X2);
	}

	//rain & snow
	for(uint8_t i = 0; i < 43; i++)
	{
		uint16_t snow = forecastData->hourForecast[timeOffset + i].snow;
		uint16_t startSnow = (-51*snow)/(precipitationMax) + 125;
		if(snow != 0)
		{
			Paint_DrawRectangle(4*i+33, startSnow, 4*i+34, 125, BLACK, DRAW_FILL_FULL, DOT_PIXEL_1X1);	//snow
		}
		if(forecastData->hourForecast[timeOffset + i].condition == WEATHER_STORM)
		{
			Paint_DrawPoint(4*i+31, 75, WHITE, DOT_PIXEL_3X3, DOT_FILL_RIGHTUP);
		}
	}

	//pressure line
	for(uint8_t i = 0; i < 43; i++)
	{
		uint16_t pressure = forecastData->hourForecast[timeOffset + i].pressure;
		uint16_t start = (-51*(pressure-ranges.minPressure))/(ranges.maxPressure-ranges.minPressure) + 176;

		Paint_DrawLine(4*(i)+31, start, 4*(i)+34, start, RED, LINE_STYLE_SOLID, DOT_PIXEL_1X1);	//rysowanie poza zakresem xend == 202
	}

	//wind
	for(uint8_t i = 0; i < 43; i++)
	{
		uint16_t windG = forecastData->hourForecast[timeOffset + i].wind_gust;
		uint16_t wind = forecastData->hourForecast[timeOffset + i].wind;
		uint16_t start = (-21*windG)/(ranges.maxGWind) + 200;
		uint16_t end = (-21*wind)/(ranges.maxGWind) + 200;

		Paint_DrawRectangle(4*(i)+31, start, 4*(i)+34, end, RED, DRAW_FILL_FULL, DOT_PIXEL_1X1);
	}

	if(isCharging)
	{
		Paint_DrawRectangle(0, 0, 199, 3, RED, DRAW_FILL_FULL, DOT_PIXEL_1X1);
	}

	EPD_SendRed(d_red);
	system_sleep(2);

	EPD_Refresh();
	system_sleep(2);

}

//void drawForecast(const SForecast * const forecastData)
//{
//	SRangesValues ranges;
//	uint8_t battery = system_batteryLevel();
//	uint8_t isCharging = system_isCharging();
//
//	uint8_t d_black[(EPD_WIDTH/8) * EPD_HEIGHT];
//	uint8_t d_grey[(EPD_WIDTH/8) * EPD_HEIGHT];
//	uint8_t *d_red = d_black;
//	uint8_t startHour;
//
//	char textBuff[64];
//	time_t forecastTime;
//
//	int16_t maxTempRange;
//	int16_t minTempRange;
//	int16_t avrTempLine;
//
//	int16_t precipitationMax;
//
//	getRangeValues(&ranges, forecastData);
//
//
//	forecastTime = forecastData->hourForecast[0].time + forecastData->timeZoneOffset;
//	struct tm * timeObject = gmtime(&forecastTime);
//	startHour = timeObject->tm_hour;
//
//	//black & grey
//	Paint_NewImage(d_black, EPD_WIDTH, EPD_HEIGHT, 0, WHITE);
//	Paint_NewImage(d_grey, EPD_WIDTH, EPD_HEIGHT, 0, WHITE);
//
//	Paint_SelectImage(d_black);
//	Paint_Clear(WHITE);
//	Paint_SelectImage(d_grey);
//	Paint_Clear(WHITE);
//
//	Paint_SelectImage(d_black);
//	Paint_DrawBitMap(forecast_black);
//
//	sprintf(textBuff, "%02d/%02d", timeObject->tm_mday, timeObject->tm_mon+1);
//	Paint_DrawString(0, 8, textBuff, &Font8, WHITE, BLACK);
//
//	for(uint8_t i = 0; i < 7; i++)
//	{
//		sprintf(textBuff, "%02d", startHour);
//		Paint_DrawString(24*i + 31, 6, textBuff, &Font12, WHITE, BLACK);
//		startHour = (startHour + 6) % 24;
//	}
//
//	//TEMPERATURE
//	if((ranges.maxTemp%10))
//		ranges.maxTemp = ranges.maxTemp + (10 - ranges.maxTemp%10);
//	ranges.minTemp = 10*((ranges.minTemp/10));
//
//	if((ranges.maxFtemp%10))
//		ranges.maxFtemp = ranges.maxFtemp + (10 - ranges.maxFtemp%10);
//	ranges.minFtemp = 10*((ranges.minFtemp/10));
//
//	maxTempRange = ranges.maxTemp > ranges.maxFtemp ? ranges.maxTemp : ranges.maxFtemp;
//	minTempRange = ranges.minTemp < ranges.minFtemp ? ranges.minTemp : ranges.minFtemp;
//
//
//	sprintf(textBuff, "%3d", maxTempRange/10);
//	Paint_DrawString(8, 21, textBuff, &Font12, WHITE, BLACK);
//	sprintf(textBuff, "%3d", (maxTempRange + minTempRange)/20);
//	Paint_DrawString(8, 41, textBuff, &Font12, WHITE, BLACK);
//	sprintf(textBuff, "%3d", minTempRange/10);
//	Paint_DrawString(8, 61, textBuff, &Font12, WHITE, BLACK);
//
//	if(minTempRange < 0)
//	{
//		Paint_SelectImage(d_grey);
//		Paint_DrawRectangle(31, (51*minTempRange)/(maxTempRange-minTempRange)+74, 199, 74, BLACK, DRAW_FILL_FULL, DOT_PIXEL_1X1);
//		Paint_SelectImage(d_black);
//	}
//
//	avrTempLine = (-51*(((maxTempRange + minTempRange)/2)-minTempRange))/(maxTempRange-minTempRange) + 72;
//	Paint_DrawLine(31, avrTempLine, 199, avrTempLine, BLACK, LINE_STYLE_DOTTED, DOT_PIXEL_1X1);
//
//	for(uint8_t i = 1; i < 43; i++)
//	{
//		int16_t startTemp = forecastData->hourForecast[i-1].temp;
//		int16_t endTemp = forecastData->hourForecast[i].temp;
//
//		uint16_t start = (-51*(startTemp-minTempRange))/(maxTempRange-minTempRange) + 72;
//		uint16_t end = (-51*(endTemp-minTempRange))/(maxTempRange-minTempRange) + 72;
//
//		Paint_DrawLine(4*(i-1)+31, start, 4*(i)+31, end, BLACK, LINE_STYLE_SOLID, DOT_PIXEL_2X2);
//	}
//
//	for(uint8_t i = 1; i < 43; i++)
//	{
//		int16_t startTemp = forecastData->hourForecast[i-1].temp_feel;
//		int16_t endTemp = forecastData->hourForecast[i].temp_feel;
//
//		uint16_t start = (-51*(startTemp-minTempRange))/(maxTempRange-minTempRange) + 72;
//		uint16_t end = (-51*(endTemp-minTempRange))/(maxTempRange-minTempRange) + 72;
//
//		Paint_DrawLine(4*(i-1)+31, start, 4*(i)+31, end, BLACK, LINE_STYLE_SOLID, DOT_PIXEL_2X2);
//	}
//
//	//rain & snow
//	ranges.maxRain = ranges.maxRain + (10 - ranges.maxRain%10);
//	ranges.maxSnow = ranges.maxSnow + (10 - ranges.maxSnow%10);
//
//	precipitationMax = ranges.maxRain > ranges.maxSnow ? ranges.maxRain : ranges.maxSnow;
//
//	sprintf(textBuff, "%3d", precipitationMax/10);
//	Paint_DrawString(8, 76, textBuff, &Font12, WHITE, BLACK);
//	Paint_DrawString(8, 114, "  0", &Font12, WHITE, BLACK);
//
//	for(uint8_t i = 0; i < 43; i++)
//	{
//		uint16_t rain = forecastData->hourForecast[i].rain;
//		uint16_t snow = forecastData->hourForecast[i].snow;
//		uint16_t startRain = (-51*rain)/(precipitationMax) + 125;
//		uint16_t startSnow = (-51*snow)/(precipitationMax) + 125;
//
//		if(rain != 0)
//		{
//			Paint_DrawRectangle(4*i+31, startRain, 4*i+32, 125, BLACK, DRAW_FILL_FULL, DOT_PIXEL_1X1);	//rain
//		}
//		if(snow != 0)
//		{
//			Paint_DrawRectangle(4*i+33, startSnow, 4*i+34, 125, BLACK, DRAW_FILL_FULL, DOT_PIXEL_1X1);	//snow
//		}
//		if(forecastData->hourForecast[i].condition == WEATHER_STORM)
//		{
//			Paint_DrawPoint(4*i+31, 75, WHITE, DOT_PIXEL_3X3, DOT_FILL_RIGHTUP);
//			Paint_DrawPoint(4*i+31, 75, BLACK, DOT_PIXEL_3X3, DOT_FILL_RIGHTUP);
//		}
//	}
//
//	//pop
//	for(uint8_t i = 1; i < 43; i++)
//	{
//		int16_t startPOP = forecastData->hourForecast[i-1].pop;
//		int16_t endPOP = forecastData->hourForecast[i].pop;
//
//		uint16_t start = (-51*(startPOP))/(100) + 125;
//		uint16_t end = (-51*(endPOP))/(100) + 125;
//
//		Paint_DrawLine(4*(i-1)+31, start, 4*(i)+31, end, BLACK, LINE_STYLE_SOLID, DOT_PIXEL_1X1);
//	}
//
//	//PRESSURE
//	ranges.maxPressure = ranges.maxPressure + (5 - ranges.maxPressure%5);
//	ranges.minPressure = ranges.minPressure - (ranges.minPressure%5);
//
//	sprintf(textBuff, "%4d", ranges.maxPressure);
//	Paint_DrawString(1, 126, textBuff, &Font12, WHITE, BLACK);
//	sprintf(textBuff, "%4d", ranges.minPressure);
//	Paint_DrawString(1, 165, textBuff, &Font12, WHITE, BLACK);
//
//	for(uint8_t i = 0; i < 43; i++)
//	{
//		uint16_t pressure = forecastData->hourForecast[i].pressure;
//		uint16_t start = (-51*(pressure-ranges.minPressure))/(ranges.maxPressure-ranges.minPressure) + 177;
//
//		Paint_SelectImage(d_grey);
//		Paint_DrawRectangle(4*(i)+31, start, 4*(i)+34, 177, BLACK, DRAW_FILL_FULL, DOT_PIXEL_1X1);
//
//		Paint_SelectImage(d_black);
//		Paint_DrawLine(4*(i)+31, start, 4*(i)+34, start, BLACK, LINE_STYLE_SOLID, DOT_PIXEL_1X1);
//	}
//
//
//	//wind
//	if(ranges.maxGWind < 1) ranges.maxGWind = 1;
//	sprintf(textBuff, "%2d", ranges.maxGWind);
//	Paint_DrawString(15, 178, textBuff, &Font12, WHITE, BLACK);
//	Paint_DrawString(15, 188, " 0", &Font12, WHITE, BLACK);
//
//	for(uint8_t i = 0; i < 43; i++)
//	{
//		uint16_t windG = forecastData->hourForecast[i].wind_gust;
//		uint16_t wind = forecastData->hourForecast[i].wind;
//
//		uint16_t start = (-21*windG)/(ranges.maxGWind) + 200;
//		uint16_t end = (-21*wind)/(ranges.maxGWind) + 200;
//
//		Paint_SelectImage(d_black);
//		Paint_DrawRectangle(4*(i)+31, start, 4*(i)+34, end, BLACK, DRAW_FILL_FULL, DOT_PIXEL_1X1);
//		Paint_SelectImage(d_grey);
//		Paint_DrawRectangle(4*(i)+31, end, 4*(i)+34, 200, BLACK, DRAW_FILL_FULL, DOT_PIXEL_1X1);
//
//	}
//
//	Paint_SelectImage(d_black);
//
//	if(!isCharging)
//	{
//		Paint_DrawRectangle(0, 0, battery*2, 3, BLACK, DRAW_FILL_FULL, DOT_PIXEL_1X1);
//	}
//	else
//	{
//		Paint_DrawRectangle(0, 0, 199, 3, RED, DRAW_FILL_FULL, DOT_PIXEL_1X1);
//	}
//
//	EPD_SendBlackAndGrey(d_black, d_grey);
//
//	//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
//	//red
//	Paint_SelectImage(d_red);
//	Paint_Clear(WHITE);
//	Paint_DrawBitMap(forecast_red);
//
//	//feel temp
//	for(uint8_t i = 1; i < 43; i++)
//	{
//		int16_t startTemp = forecastData->hourForecast[i-1].temp_feel;
//		int16_t endTemp = forecastData->hourForecast[i].temp_feel;
//
//		uint16_t start = (-51*(startTemp-minTempRange))/(maxTempRange-minTempRange) + 72;
//		uint16_t end = (-51*(endTemp-minTempRange))/(maxTempRange-minTempRange) + 72;
//
//		Paint_DrawLine(4*(i-1)+31, start, 4*(i)+31, end, RED, LINE_STYLE_SOLID, DOT_PIXEL_2X2);
//	}
//
//	//rain & snow
//	for(uint8_t i = 0; i < 43; i++)
//	{
//		uint16_t snow = forecastData->hourForecast[i].snow;
//		uint16_t startSnow = (-51*snow)/(precipitationMax) + 125;
//		if(snow != 0)
//		{
//			Paint_DrawRectangle(4*i+33, startSnow, 4*i+34, 125, BLACK, DRAW_FILL_FULL, DOT_PIXEL_1X1);	//snow
//		}
//		if(forecastData->hourForecast[i].condition == WEATHER_STORM)
//		{
//			Paint_DrawPoint(4*i+31, 75, WHITE, DOT_PIXEL_3X3, DOT_FILL_RIGHTUP);
//		}
//	}
//
//	//pressure line
//	for(uint8_t i = 0; i < 43; i++)
//	{
//		uint16_t pressure = forecastData->hourForecast[i].pressure;
//		uint16_t start = (-51*(pressure-ranges.minPressure))/(ranges.maxPressure-ranges.minPressure) + 176;
//
//		Paint_DrawLine(4*(i)+31, start, 4*(i)+34, start, RED, LINE_STYLE_SOLID, DOT_PIXEL_1X1);
//	}
//
//	//wind
//	for(uint8_t i = 0; i < 43; i++)
//	{
//		uint16_t windG = forecastData->hourForecast[i].wind_gust;
//		uint16_t wind = forecastData->hourForecast[i].wind;
//		uint16_t start = (-21*windG)/(ranges.maxGWind) + 200;
//		uint16_t end = (-21*wind)/(ranges.maxGWind) + 200;
//
//		Paint_DrawRectangle(4*(i)+31, start, 4*(i)+34, end, RED, DRAW_FILL_FULL, DOT_PIXEL_1X1);
//	}
//
//	if(isCharging)
//	{
//		Paint_DrawRectangle(0, 0, 199, 3, RED, DRAW_FILL_FULL, DOT_PIXEL_1X1);
//	}
//
//	EPD_SendRed(d_red);
//	system_sleep(2);
//
//	EPD_Refresh();
//	system_sleep(2);
//
//}

static void getRangeValues2(SRangesValues *rangeValues, const SForecast * const forecastData, uint8_t offset)
{
	rangeValues->maxTemp = INT16_MIN;
	rangeValues->minTemp = INT16_MAX;

	rangeValues->maxFtemp = INT16_MIN;
	rangeValues->minFtemp = INT16_MAX;

	rangeValues->maxRain = 0;
	rangeValues->maxSnow = 0;

	rangeValues->maxPressure = 0;
	rangeValues->minPressure = UINT16_MAX;

	rangeValues->maxWind = 0;
	rangeValues->maxGWind = 0;

	if(offset > FORECAST_SIZE - DISPLAY_DATA_SIZE)
		return;

	for(uint8_t i = offset; i < DISPLAY_DATA_SIZE; i++)
	{
		SForecastHour hourF = forecastData->hourForecast[i];

		if(hourF.temp > rangeValues->maxTemp) rangeValues->maxTemp = hourF.temp;
		if(hourF.temp < rangeValues->minTemp) rangeValues->minTemp = hourF.temp;

		if(hourF.temp_feel > rangeValues->maxFtemp) rangeValues->maxFtemp = hourF.temp_feel;
		if(hourF.temp_feel < rangeValues->minFtemp) rangeValues->minFtemp = hourF.temp_feel;

		if(hourF.rain > rangeValues->maxRain) rangeValues->maxRain = hourF.rain;
		if(hourF.snow > rangeValues->maxSnow) rangeValues->maxSnow = hourF.snow;

		if(hourF.pressure > rangeValues->maxPressure) rangeValues->maxPressure = hourF.pressure;
		if(hourF.pressure < rangeValues->minPressure) rangeValues->minPressure = hourF.pressure;

		if(hourF.wind > rangeValues->maxWind) rangeValues->maxWind = hourF.wind;
		if(hourF.wind_gust > rangeValues->maxGWind) rangeValues->maxGWind = hourF.wind_gust;

	}
}

//static void getRangeValues(SRangesValues *rangeValues, const SForecast * const forecastData)
//{
//	rangeValues->maxTemp = INT16_MIN;
//	rangeValues->minTemp = INT16_MAX;
//
//	rangeValues->maxFtemp = INT16_MIN;
//	rangeValues->minFtemp = INT16_MAX;
//
//	rangeValues->maxRain = 0;
//	rangeValues->maxSnow = 0;
//
//	rangeValues->maxPressure = 0;
//	rangeValues->minPressure = UINT16_MAX;
//
//	rangeValues->maxWind = 0;
//	rangeValues->maxGWind = 0;
//
////	FORECAST_SIZE
//	for(uint8_t i = 0; i < 48; i++)
//	{
//		SForecastHour hourF = forecastData->hourForecast[i];
//
//		if(hourF.temp > rangeValues->maxTemp) rangeValues->maxTemp = hourF.temp;
//		if(hourF.temp < rangeValues->minTemp) rangeValues->minTemp = hourF.temp;
//
//		if(hourF.temp_feel > rangeValues->maxFtemp) rangeValues->maxFtemp = hourF.temp_feel;
//		if(hourF.temp_feel < rangeValues->minFtemp) rangeValues->minFtemp = hourF.temp_feel;
//
//		if(hourF.rain > rangeValues->maxRain) rangeValues->maxRain = hourF.rain;
//		if(hourF.snow > rangeValues->maxSnow) rangeValues->maxSnow = hourF.snow;
//
//		if(hourF.pressure > rangeValues->maxPressure) rangeValues->maxPressure = hourF.pressure;
//		if(hourF.pressure < rangeValues->minPressure) rangeValues->minPressure = hourF.pressure;
//
//		if(hourF.wind > rangeValues->maxWind) rangeValues->maxWind = hourF.wind;
//		if(hourF.wind_gust > rangeValues->maxGWind) rangeValues->maxGWind = hourF.wind_gust;
//
//	}
//}
