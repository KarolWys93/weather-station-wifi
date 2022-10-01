/*
 * utils.c
 *
 *  Created on: 01.05.2022
 *      Author: Karol
 */

#include "utils.h"
#include "../wifi/wifi_esp.h"
#include "logger.h"
#include "rtc.h"
#include "system.h"

#define OLD_TIME_STAMP 946681200	//00:00:00 01-01-2000


bool timeSync(uint8_t retries)
{
	uint8_t retryCounter = 0;
	time_t time = 0;

	while(retryCounter <= retries)
	{
		Logger(LOG_INF, "Time sync attempt %d", retryCounter+1);
		if(WIFI_RESP_OK == WiFi_getSNTPtime(&time, 1000))
		{
			if(OLD_TIME_STAMP < time)
			{
				RTC_setTime(time);
				Logger(LOG_INF, "Sync time OK %d", time);
				return true;
			}
		}
		else
		{
			Logger(LOG_WRN, "get time FAILED");
		}

		if(0 == retries) { break; }

		system_sleep(1000);
		retryCounter++;
	}
	return false;
}

bool waitForConnection(uint8_t retries)
{
	uint8_t retryCounter = retries;

	WiFi_UpdateStatus(1000);
	while(!wifiStatus.connectedToAP)
	{
		if(!retryCounter) break;
		system_sleep(1000);
		WiFi_UpdateStatus(1000);
		retryCounter--;
	};

	return wifiStatus.connectedToAP;
}

