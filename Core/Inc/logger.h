/*
 * logger.h
 *
 *  Created on: 09.04.2022
 *      Author: Karol
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <stdint.h>

#define LOG_PATH "/log"
#define LOG_FILE_NAME "station.log"
#define LOG_MAX_FILE_SIZE (512*1024)    //in bytes
#define LOG_FILE_ROTATION 3

#define LOG_MAX_LINE_SIZE 128

#define LOG_SYNC_TIME 0 * 1000    //in msec

#define LOGGER_USE_UART

#ifdef LOGGER_USE_UART
	#include "usart.h"
	#define LOG_UART huart2
#endif

typedef enum Log_Level
{
	LOG_DBG = 0,
	LOG_INF,
	LOG_WRN,
	LOG_ERR,
	LOG_VIP
} Log_Level;

uint8_t Logger_init(void);
void Logger_shutdown(void);

uint8_t Logger_pause(uint8_t pause);

void Logger_setMinLevel(Log_Level level);
Log_Level Logger_getMinLevel(void);

void Logger_sync(void);

void Logger(Log_Level level, char* fmt, ...);

#endif /* LOGGER_H_ */
