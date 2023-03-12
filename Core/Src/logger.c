/*
 * logger.c
 *
 *  Created on: 09.04.2022
 *      Author: Karol
 */

#include "logger.h"
#include "fatfs.h"
#include "rtc.h"
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#ifdef LOGGER_USE_UART
#include "string.h"
#endif

static FIL logFile;
static Log_Level currentLogLvl;
static uint32_t lastSyncStamp;
static uint8_t logInitialized = 0;
static uint8_t logPaused = 0;

static char buffer[LOG_MAX_LINE_SIZE] = "";

#if LOG_FILE_ROTATION < 1
 #error "LOG_FILE_ROTATION cannot be less than 1"
#endif

static const char* levelStr[] =
{
		"DBG",
		"INF",
		"WRN",
		"ERR",
		"VIP"
};

static void logRotation(void)
{
	char newFilePath[64];
	char oldFilePath[64];

	f_close(&logFile);

	sprintf(oldFilePath, "%s/%d_%s", LOG_PATH, LOG_FILE_ROTATION, LOG_FILE_NAME);
	f_unlink(oldFilePath);	//remove the oldest logs

	for(uint8_t i = LOG_FILE_ROTATION - 1; i > 0 ; i--)
	{
		sprintf(oldFilePath, "%s/%d_%s", LOG_PATH, i, LOG_FILE_NAME);
		sprintf(newFilePath, "%s/%d_%s", LOG_PATH, i+1, LOG_FILE_NAME);
		f_rename(oldFilePath, newFilePath);
	}
	sprintf(oldFilePath, "%s/%s", LOG_PATH, LOG_FILE_NAME);
	sprintf(newFilePath, "%s/%d_%s", LOG_PATH, 1, LOG_FILE_NAME);
	f_rename(oldFilePath, newFilePath);
	f_open(&logFile, oldFilePath, FA_OPEN_ALWAYS|FA_WRITE);
}

uint8_t Logger_init(void)
{
	FRESULT result;

	currentLogLvl = LOG_INF;
	f_mkdir (LOG_PATH);

	result = f_open(&logFile, LOG_PATH"/"LOG_FILE_NAME, FA_OPEN_ALWAYS|FA_WRITE);
	if(result != FR_OK)
	{
		return result;
	}
	f_lseek(&logFile, f_size(&logFile));

	lastSyncStamp = 0;

	logInitialized = 1;
	logPaused = 0;
	Logger(LOG_VIP, "=-=-=-=-=-=");
	Logger(LOG_INF, "Logger started");

	return result;
}

void Logger_shutdown(void)
{
	Logger(LOG_VIP, "Logger shutdown");
	f_sync(&logFile);
	f_close(&logFile);
	logInitialized = 0;
	logPaused = 0;
}

uint8_t Logger_pause(uint8_t pause)
{
	FRESULT result;
	if(!logInitialized)
	{
		return 1;
	}
	if(pause)
	{
		f_close(&logFile);
		logPaused = 1;
	}
	else
	{
		result = f_open(&logFile, LOG_PATH"/"LOG_FILE_NAME, FA_WRITE);
		if(result != FR_OK)
		{
			return 1;
		}
		f_lseek(&logFile, f_size(&logFile));
		logPaused = 0;
	}
	return 0;
}

void Logger_setMinLevel(Log_Level level)
{
	currentLogLvl = (level > LOG_VIP) ? LOG_VIP : level;
}

Log_Level Logger_getMinLevel(void)
{
	return currentLogLvl;
}

void Logger_sync(void)
{
	if(!logInitialized || logPaused) return;

	f_sync(&logFile);

	if(f_size(&logFile) >= LOG_MAX_FILE_SIZE)
	{
		logRotation();
	}

	lastSyncStamp = HAL_GetTick();
}

void Logger(Log_Level level, char* fmt, ...)
{
	struct tm * t;
	time_t currentTime = 0;

	if(currentLogLvl > level)
	{
		return;
	}

#ifdef LOGGER_USE_UART
	while(HAL_UART_STATE_READY != HAL_UART_GetState(&LOG_UART)) {};
#endif

	va_list args;
	va_start (args, fmt);
	vsnprintf(buffer, LOG_MAX_LINE_SIZE-1, fmt, args);
	perror (buffer);
	va_end (args);

#ifdef LOGGER_USE_UART
//TODO: send \r\n in interrupt after sending is over
//	if(LOG_MAX_LINE_SIZE-3 > strlen(buffer))
//	{
//		strcat(buffer, "\r\n");
//	}
	HAL_UART_Transmit_DMA(&LOG_UART, (uint8_t*)buffer, strlen(buffer));
#endif

	if(!logInitialized || logPaused) return;

	//1970/10/23 00:46:46 INF log text
	currentTime = RTC_getTime();
	t = gmtime(&currentTime);
	f_printf(&logFile, "%d/%02d/%02d %02d:%02d:%02d %s %s\n",
			t->tm_year + 1900,
			t->tm_mon + 1,
			t->tm_mday,
			t->tm_hour,
			t->tm_min,
			t->tm_sec,
			levelStr[level],
			buffer);

	if(HAL_GetTick() - lastSyncStamp > LOG_SYNC_TIME)
	{
		Logger_sync();
	}
}

