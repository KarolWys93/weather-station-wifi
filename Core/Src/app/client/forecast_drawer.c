/*
 * forecast_drawer.c
 *
 *  Created on: 01.05.2022
 *      Author: Karol
 */

#include "e-Paper/Config/DEV_Config.h"
#include "forecast_drawer.h"

#include "e-Paper/EPD_1in54b.h"
#include "GUI/GUI_Paint.h"
#include "e-Paper/ImageData.h"

#include "rtc.h"
#include <stdio.h>

#include "system.h"
#include "logger.h"

#include "images.h"

#define DISPLAY_DATA_SIZE 42

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


static void getRangeValues(SRangesValues *rangeValues, const SForecast * const forecastData, uint8_t offset);
static inline int16_t tempRoundUp(int16_t temp);
static inline int16_t tempRoundDown(int16_t temp);

void drawForecast(const SForecast * const forecastData)
{
    SRangesValues ranges;
    uint8_t battery = system_batteryLevel();
    uint8_t isCharging = system_powerStatus();

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

    getRangeValues(&ranges, forecastData, timeOffset);

    load_forecastImg_BlackGrey(d_black, d_grey);

    Paint_SelectImage(d_black);

    sprintf(textBuff, "%02d/%02d", timeObject->tm_mday, timeObject->tm_mon+1);
    Paint_DrawString(1, 8, textBuff, &Font8, WHITE, BLACK);

    for(uint8_t i = 0; i < 7; i++)
    {
        sprintf(textBuff, "%02d", startHour);
        Paint_DrawString(24*i + 31, 6, textBuff, &Font12, WHITE, BLACK);
        startHour = (startHour + 6) % 24;
    }

    for(uint8_t i = 0; i < DISPLAY_DATA_SIZE; i++)
    {
        if((forecastData->hourForecast[i + timeOffset].time + forecastData->timeZoneOffset)%(3600 * 24) == 0)
        {
            Paint_DrawLine(4*(i)+31, 21, 4*(i)+31, 200, BLACK, LINE_STYLE_DOTTED, DOT_PIXEL_1X1);
        }
    }

    //TEMPERATURE
    ranges.maxTemp = tempRoundUp(ranges.maxTemp);
    ranges.minTemp = tempRoundDown(ranges.minTemp);

    ranges.maxFtemp = tempRoundUp(ranges.maxFtemp);
    ranges.minFtemp = tempRoundDown(ranges.minFtemp);

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
        Paint_DrawRectangle(31, (51*minTempRange)/(maxTempRange-minTempRange)+73, 199, 73, BLACK, DRAW_FILL_FULL, DOT_PIXEL_1X1);
        Paint_SelectImage(d_black);
    }

    avrTempLine = (-51*(((maxTempRange + minTempRange)/2)-minTempRange))/(maxTempRange-minTempRange) + 72;
    Paint_DrawLine(31, avrTempLine, 199, avrTempLine, BLACK, LINE_STYLE_DOTTED, DOT_PIXEL_1X1);

    for(uint8_t i = 0; i < DISPLAY_DATA_SIZE; i++)
    {
        int16_t startTemp = forecastData->hourForecast[timeOffset + i].temp;
        int16_t endTemp = forecastData->hourForecast[timeOffset + i + 1].temp;

        uint16_t start = (-51*(startTemp-minTempRange))/(maxTempRange-minTempRange) + 72;
        uint16_t end = (-51*(endTemp-minTempRange))/(maxTempRange-minTempRange) + 72;

        Paint_DrawLine(4*(i)+31, start, 4*(i+1)+31, end, BLACK, LINE_STYLE_SOLID, DOT_PIXEL_2X2);
    }

    for(uint8_t i = 0; i < DISPLAY_DATA_SIZE; i++)
    {
        int16_t startTemp = forecastData->hourForecast[timeOffset + i].temp_feel;
        int16_t endTemp = forecastData->hourForecast[timeOffset + i + 1].temp_feel;

        uint16_t start = (-51*(startTemp-minTempRange))/(maxTempRange-minTempRange) + 72;
        uint16_t end = (-51*(endTemp-minTempRange))/(maxTempRange-minTempRange) + 72;

        Paint_DrawLine(4*(i)+31, start, 4*(i+1)+31, end, BLACK, LINE_STYLE_SOLID, DOT_PIXEL_2X2);
    }

    //rain & snow
    ranges.maxRain = ranges.maxRain + (10 - ranges.maxRain%10);
    ranges.maxSnow = ranges.maxSnow + (10 - ranges.maxSnow%10);

    precipitationMax = ranges.maxRain > ranges.maxSnow ? ranges.maxRain : ranges.maxSnow;

    sprintf(textBuff, "%3d", precipitationMax/10);
    Paint_DrawString(8, 75, textBuff, &Font12, WHITE, BLACK);
    Paint_DrawString(8, 114, "  0", &Font12, WHITE, BLACK);

    for(uint8_t i = 0; i < DISPLAY_DATA_SIZE; i++)
    {
        uint16_t rain = forecastData->hourForecast[timeOffset + i].rain;
        uint16_t snow = forecastData->hourForecast[timeOffset + i].snow;
        uint16_t startRain = (-50*rain)/(precipitationMax) + 125;
        uint16_t startSnow = (-50*snow)/(precipitationMax) + 125;

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
            Paint_DrawPoint(4*i+31-2, 75, WHITE, DOT_PIXEL_6X6, DOT_FILL_RIGHTUP);
            Paint_DrawPoint(4*i+31-2, 75, BLACK, DOT_PIXEL_6X6, DOT_FILL_RIGHTUP);
        }
    }

    //pressure
    ranges.maxPressure = ranges.maxPressure + (5 - ranges.maxPressure%5);
    ranges.minPressure = ranges.minPressure - (ranges.minPressure%5);

    sprintf(textBuff, "%4d", ranges.maxPressure);
    Paint_DrawString(1, 127, textBuff, &Font12, WHITE, BLACK);
    sprintf(textBuff, "%4d", ranges.minPressure);
    Paint_DrawString(1, 165, textBuff, &Font12, WHITE, BLACK);

    for(uint8_t i = 0; i < DISPLAY_DATA_SIZE; i++)
    {
        uint16_t pressure = forecastData->hourForecast[timeOffset + i].pressure;
        uint16_t start = (-50*(pressure-ranges.minPressure))/(ranges.maxPressure-ranges.minPressure) + 177;

        Paint_SelectImage(d_grey);
        Paint_DrawRectangle(4*(i)+31, start, 4*(i)+34, 177, BLACK, DRAW_FILL_FULL, DOT_PIXEL_1X1);

        Paint_SelectImage(d_black);
        Paint_DrawLine(4*(i)+31, start, 4*(i)+34, start, BLACK, LINE_STYLE_SOLID, DOT_PIXEL_1X1);
    }


    //wind
    if(ranges.maxGWind < 1) ranges.maxGWind = 1;
    sprintf(textBuff, "%2d", ranges.maxGWind);
    Paint_DrawString(15, 179, textBuff, &Font12, WHITE, BLACK);
    Paint_DrawString(15, 188, " 0", &Font12, WHITE, BLACK);

    for(uint8_t i = 0; i < DISPLAY_DATA_SIZE; i++)
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

    draw_battery_level_black(d_black, battery, isCharging);

    save_img_BlackGrey(d_black, d_grey);
    EPD_SendBlackAndGrey(d_black, d_grey);

    //=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

    //red
    Paint_SelectImage(d_red);
    load_forecastImg_Red(d_red);
    //	Paint_Clear(WHITE);
    //	Paint_DrawBitMap(forecast_red);

    //feel temp
    for(uint8_t i = 0; i < DISPLAY_DATA_SIZE; i++)
    {
        int16_t startTemp = forecastData->hourForecast[timeOffset + i].temp_feel;
        int16_t endTemp = forecastData->hourForecast[timeOffset + i + 1].temp_feel;

        uint16_t start = (-51*(startTemp-minTempRange))/(maxTempRange-minTempRange) + 72;
        uint16_t end = (-51*(endTemp-minTempRange))/(maxTempRange-minTempRange) + 72;

        Paint_DrawLine(4*(i)+31, start, 4*(i+1)+31, end, RED, LINE_STYLE_SOLID, DOT_PIXEL_2X2);
    }

    //rain & snow
    for(uint8_t i = 0; i < DISPLAY_DATA_SIZE; i++)
    {
        uint16_t snow = forecastData->hourForecast[timeOffset + i].snow;
        uint16_t startSnow = (-50*snow)/(precipitationMax) + 125;
        if(snow != 0)
        {
            Paint_DrawRectangle(4*i+33, startSnow, 4*i+34, 125, BLACK, DRAW_FILL_FULL, DOT_PIXEL_1X1);	//snow
        }

        if(forecastData->hourForecast[timeOffset + i].condition == WEATHER_STORM)
        {
            Paint_DrawPoint(4*i+31-2, 75, WHITE, DOT_PIXEL_6X6, DOT_FILL_RIGHTUP);
            Paint_DrawPoint(4*i+31-2, 75, BLACK, DOT_PIXEL_6X6, DOT_FILL_RIGHTUP);
        }
    }

    //wind
    for(uint8_t i = 0; i < DISPLAY_DATA_SIZE; i++)
    {
        uint16_t windG = forecastData->hourForecast[timeOffset + i].wind_gust;
        uint16_t wind = forecastData->hourForecast[timeOffset + i].wind;
        uint16_t start = (-21*windG)/(ranges.maxGWind) + 200;
        uint16_t end = (-21*wind)/(ranges.maxGWind) + 200;

        Paint_DrawRectangle(4*(i)+31, start, 4*(i)+34, end, RED, DRAW_FILL_FULL, DOT_PIXEL_1X1);
    }


    draw_battery_level_red(d_red, battery, isCharging);

    save_img_Red(d_red);
    EPD_SendRed(d_red);
    system_sleep(1);

    EPD_Refresh();
}

static void getRangeValues(SRangesValues *rangeValues, const SForecast * const forecastData, uint8_t offset)
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

    if(offset > FORECAST_SIZE - DISPLAY_DATA_SIZE+1)
        return;

    for(uint8_t i = 0; i < DISPLAY_DATA_SIZE+1; i++)
    {
        SForecastHour hourF = forecastData->hourForecast[i + offset];

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

    Logger(LOG_DBG, "Range temp [%d:%d]", rangeValues->minTemp, rangeValues->maxTemp);
    Logger(LOG_DBG, "Range temp_feel [%d:%d]", rangeValues->minFtemp, rangeValues->maxFtemp);
    Logger(LOG_DBG, "Range rain/snow [%d:%d]", rangeValues->maxRain, rangeValues->maxSnow);
    Logger(LOG_DBG, "Range pressure [%d:%d]", rangeValues->minPressure, rangeValues->maxPressure);
    Logger(LOG_DBG, "Range wind/gust [%d:%d]", rangeValues->maxWind, rangeValues->maxGWind);
}

static inline int16_t tempRoundUp(int16_t temp)
{
    if((temp%10))
    {
        if(temp > 0)
        {
            temp = temp + (10 - temp%10);
        }
        else
        {
            temp = 10*((temp/10));
        }
    }

    return temp;
}

static inline int16_t tempRoundDown(int16_t temp)
{
    return -tempRoundUp(-temp);
}
