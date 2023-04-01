/*
 * forecast_client.c
 *
 *  Created on: 01.05.2022
 *      Author: Karol
 */

#include "forecast_client.h"
#include "forecast.h"
#include "utils.h"

#include "http_requests.h"
#include "jsmn.h"
#include "logger.h"
#include "../wifi/wifi_esp.h"

#include "main.h"
#include "images.h"
#include "fatfs.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "rtc.h"

#include "forecast_drawer.h"
#include "system.h"

#include "led.h"

#include "sw_watchdog.h"

#define RETRY_TIME 5 //min

typedef struct SForecastConfig
{
	uint8_t updatePeriod;
	char    apiKey[33];
	char    lat[11];
	char    lon[11];
	char    zipCode[12];
	char    country[3];
} SForecastConfig;

static uint8_t readForecastConfig(SForecastConfig* forecastConfig);
static int16_t sendGetRequest(char* host, uint16_t port, char* path, char* dataBuff, uint16_t dataBuffSize);
static uint8_t geolocationRequest(SForecastConfig *forecastConfig);
static uint8_t forecastRequest(SForecastConfig* forecastConfig);

void runForecastApp(void)
{
	time_t time;
	struct tm * timeStruct;
	uint8_t result = 0;
    SForecastConfig forecastConf;
    SForecast forecast;

    sw_watchdog_reset();

	if(!waitForConnection(10))
	{
		Logger(LOG_WRN, "Cannot connect to WiFi");
		system_setWakeUpTimer(RETRY_TIME * 60);  //restart after 5 minuts
		led_setColor(LED_RED);
		show_error_image(ERR_IMG_WIFI, "NO WIFI");
		return;
	}

	timeSync(5);

	if(OLD_TIME_STAMP >= RTC_getTime())
	{
		Logger(LOG_ERR, "time failed!");
		system_setWakeUpTimer(RETRY_TIME * 60);  //restart after 5 minuts
		led_setColor(LED_RED);
		show_error_image(ERR_IMG_GENERAL, "Time error");
		return;
	}

	Logger(LOG_INF, "Start forecast app");

	if(0 != (result = readForecastConfig(&forecastConf)))
	{
		Logger(LOG_ERR, "Cannot read forecast config (%d)", result);
		led_setColor(LED_RED);
		show_error_image(ERR_IMG_GENERAL, "forecast conf err");
		return;
	}

	//geolocation
	if(strlen(forecastConf.lat) == 0 || strlen(forecastConf.lon) == 0)
	{
		Logger(LOG_INF, "Geocoding request");
		if(0 != (result = geolocationRequest(&forecastConf)))
		{
			Logger(LOG_WRN, "geolocation failed: %d", result);
			if(result == 2)
			{
				led_setColor(LED_RED);
				show_error_image(ERR_IMG_WIFI, "CONNECTION ERR");
				system_setWakeUpTimer(RETRY_TIME * 60);  //restart after 5 minuts
			}
			else
			{
				led_setColor(LED_RED);
				show_error_image(ERR_IMG_GENERAL, "geolocation error");
			}
			return;
		}
	}

	Logger(LOG_INF, "forecast for: %s, %s", forecastConf.lat, forecastConf.lon);

	//get new forecast
    f_unlink(FILE_PATH_FCAST_TMP);
    result = forecastRequest(&forecastConf);

    if(result != 0)
    {
    	Logger(LOG_WRN, "forecast req failed: %d", result);
    	if(result == 1)
    	{
    		led_setColor(LED_RED);
    		show_error_image(ERR_IMG_WIFI, "CONNECTION ERR");
    		system_setWakeUpTimer(RETRY_TIME * 60);
    	}
    	else
    	{
    		led_setColor(LED_RED);
    		show_error_image(ERR_IMG_GENERAL, "forecast req error");
    	}
    	return;
    }

    sw_watchdog_reset();

    //forecast parsing
    result = parseForecast(FILE_PATH_FCAST_TMP, &forecast);
    Logger(LOG_INF, "forecast parsing: %d", result);
    if(result != 0)
    {
    	led_setColor(LED_RED);
    	show_error_image(ERR_IMG_GENERAL, "forecast error");
    	return;
    }

    //draw forecast
    drawForecast(&forecast);
    Logger(LOG_INF, "Forecast done");

    time = RTC_getTime();
    timeStruct = gmtime(&time);
    if(timeStruct->tm_min > 15)
    {
    	system_setWakeUpTimer((60 + 5 - timeStruct->tm_min) * 60);
    }
    else
    {
    	system_setWakeUpTimer(forecastConf.updatePeriod * 60 * 60);
    }
}

static uint8_t forecastRequest(SForecastConfig* forecastConfig)
{
    char dataBuff[2048];
    char requestPath[360];
	int8_t linkID;
    char* msgPayload;
    int16_t msgSize = 0;
    int16_t dataRecLen = 0;
    UINT bw;
    FIL file;

    char startDateStr[48];
    char endDateStr[48];
    time_t time = RTC_getTime();
    struct tm * t = gmtime(&time);

    sprintf(startDateStr, "%u-%02u-%02u", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
    time += (2*24*60*60); //add 2 days
    gmtime(&time);
    sprintf(endDateStr, "%d-%02d-%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);

    sprintf(requestPath,
    		"/v1/forecast?latitude=%s&longitude=%s"
    		"&hourly=temperature_2m,apparent_temperature,rain,showers,snowfall,"
    		"weathercode,pressure_msl,windspeed_10m,windgusts_10m"
    		"&timeformat=unixtime&timezone=auto&start_date=%s&end_date=%s",
			forecastConfig->lat, forecastConfig->lon,
			startDateStr, endDateStr);

    HTTP_createRequestHeaderVer(dataBuff, 2048, HTTP_GET, requestPath, 0, HTTP_1_0);
    msgSize = HTTP_addHeaderField(dataBuff, 2048, "Host","api.open-meteo.com");
    Logger(LOG_DBG, "fcast request: %d", msgSize);


    linkID = WiFi_OpenTCPConnection("api.open-meteo.com", 80, 0);
    if(linkID < 0)
    {
    	return 1;
    }

    //request send
    WiFi_SendData(linkID, (uint8_t*) dataBuff, msgSize);

    //first part receive
    system_sleep(100);
    WiFi_UpdateStatus(1000);
    while(wifiStatus.linksStatus[linkID].dataWaiting <= 0 && wifiStatus.linksStatus[linkID].connected)
    {
    	system_sleep(100);
    	WiFi_UpdateStatus(1000);
    }

    dataRecLen = wifiStatus.linksStatus[linkID].dataWaiting;
    if(dataRecLen <= 0)
    {
    	Logger(LOG_ERR, "No data received");
    	return 1;
    }
    else if(dataRecLen > 2048)
    {
    	dataRecLen = 2048;
    }
    WiFi_ReadData(linkID, (uint8_t*) dataBuff, dataRecLen, 1000);

    if(HTTP_getResponseCode(dataBuff, dataRecLen) != 200)
    {
    	Logger(LOG_ERR, "Forecast response code: %d", HTTP_getResponseCode(dataBuff, dataRecLen));
    	return 2;
    }

    msgSize = 0;
    msgPayload = HTTP_getContent(dataBuff, dataRecLen);

    if(FR_OK == f_open(&file, FILE_PATH_FCAST_TMP, FA_OPEN_ALWAYS | FA_WRITE))
    {
    	FRESULT result;
    	result = f_write(&file, msgPayload, dataRecLen - (msgPayload-dataBuff), &bw);
    	if(FR_OK != result)
    	{
    		f_close(&file);
    		WiFi_CloseConnection(linkID);
    		return 3;
    	}

    	msgSize += (dataRecLen - (msgPayload-dataBuff));

    	while(1)
    	{
    		WiFi_UpdateStatus(1000);
    	    while(wifiStatus.linksStatus[linkID].dataWaiting <= 0 && wifiStatus.linksStatus[linkID].connected)
    	    {
    	    	system_sleep(100);
    	    	WiFi_UpdateStatus(1000);
    	    }

    	    dataRecLen = wifiStatus.linksStatus[linkID].dataWaiting;
    	    if(dataRecLen <= 0)
    	    {
    	    	break;
    	    }

    	    if(dataRecLen > 2048) dataRecLen = 2048;

    	    WiFi_ReadData(linkID, (uint8_t*) dataBuff, dataRecLen, 1000);
        	result = f_write(&file, dataBuff, dataRecLen, &bw);
        	if(FR_OK != result)
        	{
        		f_close(&file);
        		return 3;
        	}

        	msgSize += dataRecLen;

    	}
    	Logger(LOG_INF, "Forecast size: %d", msgSize);
    	f_close(&file);

    }
    else
    {
    	return 3;
    }

    WiFi_CloseConnection(linkID);
    return 0;
}

/**
 * Get location coordinate from zip code
 * Return codes:
 * 0 - success
 * 1 - wrong zip code or location not found
 * 2 - connection error
 * 3 - request error
 * @param forecastConfig
 * @return exit code
 */
static uint8_t geolocationRequest(SForecastConfig *forecastConfig)
{
    char dataBuff[512];
    char requestPath[74];

    char* msgPayload;
    int16_t msgSize = 0;
    int16_t dataRecLen = 0;

	jsmn_parser jsonParser;
	jsmntok_t jsonTokens[12];
	int16_t numOfTokens = 0;
	uint16_t tokenSize;
	jsmntok_t* tokenPtr;

    /* http://api.openweathermap.org/geo/1.0/zip?zip=12345678901,MX&appid={key} */

	if(strlen(forecastConfig->country) == 0 || strlen(forecastConfig->zipCode) == 0)
	{
		return 1;
	}

	//create request
    sprintf(requestPath, "/geo/1.0/zip?zip=%s,%s&appid=%s",
    		forecastConfig->zipCode,
			forecastConfig->country,
			forecastConfig->apiKey);


    dataRecLen = sendGetRequest("api.openweathermap.org", 80, requestPath, dataBuff, sizeof(dataBuff));

    if(dataRecLen < 0)
    {
    	Logger(LOG_ERR, "Get request failed with code: %d", dataRecLen);
    	return 2;
    }

    if(HTTP_getResponseCode(dataBuff, dataRecLen) != 200)
    {
    	Logger(LOG_WRN, "Geolocation response code: %d", HTTP_getResponseCode(dataBuff, dataRecLen));
    	if(HTTP_getResponseCode(dataBuff, dataRecLen) == 404)
    	{
    		return 1;
    	}
    	return 3;
    }

    msgSize = HTTP_getContentSize(dataBuff, dataRecLen);
    msgPayload = HTTP_getContent(dataBuff, dataRecLen);

    //parsing
	jsmn_init(&jsonParser);
	numOfTokens = jsmn_parse(&jsonParser, msgPayload, msgSize, jsonTokens, sizeof(jsonTokens) / sizeof(jsonTokens[0]));

	if(0 > numOfTokens || jsonTokens[0].type != JSMN_OBJECT)	//if no tokens or first token isn't object
	{
		return 3;
	}

	// N
	// |
	// S
	tokenPtr = jsmn_get_token("lat", msgPayload, jsonTokens, numOfTokens);
	if(tokenPtr == NULL){return 2;}
	tokenSize = tokenPtr->end - tokenPtr->start;
	if(tokenSize < 1 && tokenSize > 10){return 2;}
	memcpy(forecastConfig->lat, msgPayload + tokenPtr->start, tokenSize);
	forecastConfig->lat[tokenSize] = '\0';

	// W - E
	tokenPtr = jsmn_get_token("lon", msgPayload, jsonTokens, numOfTokens);
	if(tokenPtr == NULL){return 2;}
	tokenSize = tokenPtr->end - tokenPtr->start;
	if(tokenSize < 1 && tokenSize > 10){return 2;}
	memcpy(forecastConfig->lon, msgPayload + tokenPtr->start, tokenSize);
	forecastConfig->lon[tokenSize] = '\0';

	return 0;
}

static uint8_t readForecastConfig(SForecastConfig* forecastConfig)
{
    FIL configFile;
	char configJsonStr[256];
	UINT configJsonSize = 0;
	FRESULT fResult;

	jsmn_parser jsonParser;
	jsmntok_t jsonTokens[10];
	int16_t numOfTokens = 0;
	uint16_t tokenSize;
	jsmntok_t* tokenPtr;

    memset(forecastConfig, 0, sizeof(SForecastConfig));


    fResult = f_open(&configFile, FILE_PATH_FORECAST_CONFIG, FA_READ);
    if(fResult != FR_OK)
	{
		Logger(LOG_ERR, "Cannot open forecast config %d", fResult);
		return 1;
	}

    fResult = f_read(&configFile, configJsonStr, sizeof(configJsonStr), (UINT*)&configJsonSize);
    f_close(&configFile);
    if(fResult != FR_OK)
    {
    	Logger(LOG_ERR, "File read error: %d", fResult);
    	return 1;
	}

	jsmn_init(&jsonParser);
	numOfTokens = jsmn_parse(&jsonParser, configJsonStr, configJsonSize, jsonTokens, sizeof(jsonTokens) / sizeof(jsonTokens[0]));

	if(0 > numOfTokens || jsonTokens[0].type != JSMN_OBJECT)	//if no tokens or first token isn't object
	{
		return 2;
	}

	//update period
	tokenPtr = jsmn_get_token("refresh", configJsonStr, jsonTokens, numOfTokens);
	if(tokenPtr == NULL){return 2;}
	tokenSize = tokenPtr->end - tokenPtr->start;
	if(tokenSize < 1){return 2;}
	forecastConfig->updatePeriod = atoi(configJsonStr + tokenPtr->start);

	//lon
	tokenPtr = jsmn_get_token("lon", configJsonStr, jsonTokens, numOfTokens);
	if(tokenPtr != NULL)
	{
		tokenSize = tokenPtr->end - tokenPtr->start;
		if(tokenSize < 1 && tokenSize > 10){return 2;}
		memcpy(forecastConfig->lon, configJsonStr + tokenPtr->start, tokenSize);
		forecastConfig->lon[tokenSize] = '\0';

		//len
		tokenPtr = jsmn_get_token("lat", configJsonStr, jsonTokens, numOfTokens);
		if(tokenPtr == NULL){return 2;}
		tokenSize = tokenPtr->end - tokenPtr->start;
		if(tokenSize < 1 && tokenSize > 10){return 2;}
		memcpy(forecastConfig->lat, configJsonStr + tokenPtr->start, tokenSize);
		forecastConfig->lat[tokenSize] = '\0';
	}
	else
	{
		//api key
		tokenPtr = jsmn_get_token("api_key", configJsonStr, jsonTokens, numOfTokens);
		if(tokenPtr == NULL){return 2;}
		tokenSize = tokenPtr->end - tokenPtr->start;
		if(tokenSize != 32){return 2;}
		memcpy(forecastConfig->apiKey, configJsonStr + tokenPtr->start, tokenSize);
		forecastConfig->apiKey[tokenSize] = '\0';

		//zip code
		tokenPtr = jsmn_get_token("zip_code", configJsonStr, jsonTokens, numOfTokens);
		if(tokenPtr == NULL){return 2;}
		tokenSize = tokenPtr->end - tokenPtr->start;
		if(tokenSize > 11){return 2;}
		memcpy(forecastConfig->zipCode, configJsonStr + tokenPtr->start, tokenSize);
		forecastConfig->zipCode[tokenSize] = '\0';

		//country code
		tokenPtr = jsmn_get_token("country", configJsonStr, jsonTokens, numOfTokens);
		if(tokenPtr == NULL){return 2;}
		tokenSize = tokenPtr->end - tokenPtr->start;
		if(tokenSize != 2){return 2;}
		memcpy(forecastConfig->country, configJsonStr + tokenPtr->start, tokenSize);
		forecastConfig->country[tokenSize] = '\0';
	}

	return 0;
}

static int16_t sendGetRequest(char* host, uint16_t port, char* path, char* dataBuff, uint16_t dataBuffSize)
{
	int8_t linkID;
	uint16_t msgSize;
    Wifi_RespStatus wifiResp;
    uint16_t dataRecLen;

    HTTP_createRequestHeader(dataBuff, dataBuffSize, HTTP_GET, path, 0);
    msgSize = HTTP_addHeaderField(dataBuff, dataBuffSize, "Host", host);
    if(msgSize < 0){ return -1; }

    //send request
    linkID = WiFi_OpenTCPConnection(host, port, 0);
    if(linkID < 0) { return -2; }

    //request send
    wifiResp = WiFi_SendData(linkID, (uint8_t*) dataBuff, msgSize);
    if(wifiResp != WIFI_RESP_OK) { return -2; }

    //package receive
    system_sleep(250);
    WiFi_UpdateStatus(1000);
    while(wifiStatus.linksStatus[linkID].dataWaiting <= 0)
    {
    	system_sleep(250);
    	wifiResp = WiFi_UpdateStatus(1000);
    	if(wifiResp != WIFI_RESP_OK || !wifiStatus.linksStatus[linkID].connected)
    	{
    		return -2;
    	}
    }

    dataRecLen = wifiStatus.linksStatus[linkID].dataWaiting;
    if(dataRecLen > dataBuffSize) dataRecLen = dataBuffSize;
    dataRecLen = WiFi_ReadData(linkID, (uint8_t*) dataBuff, dataRecLen, 1000);
    if(dataRecLen < 0) { return -2; }

	WiFi_CloseConnection(linkID);

    return dataRecLen;
}
