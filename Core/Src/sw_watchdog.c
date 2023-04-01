/*
 * sw_watchdog.c
 *
 *  Created on: Apr 1, 2023
 *      Author: Karol
 */

#include "sw_watchdog.h"
#include "backup_registers.h"
#include "tim.h"
#include "logger.h"

#define watchdogTimer htim4
#define prescalerTimer htim3

static uint8_t watchdogIsRunning = 0;

/**
 * watchdog timer frequency: 1 kHz
 * min value is 100 ms
 * max value is 65535 ms (~1 minute)
 */
void sw_watchdog_enable(uint16_t time_ms)
{
	uint16_t wd_time = (uint16_t)(time_ms);	//resolution 1ms
	if(watchdogIsRunning) return;

	if(wd_time < 100){
		wd_time = 100;
	}

	__HAL_TIM_SET_AUTORELOAD(&watchdogTimer, wd_time - 1);
	HAL_TIM_GenerateEvent(&watchdogTimer, TIM_EVENTSOURCE_UPDATE);
	__HAL_TIM_CLEAR_IT(&watchdogTimer, TIM_IT_UPDATE);

	watchdogIsRunning = 1;

	Logger(LOG_DBG, "Watchdog start %u", wd_time);

#if defined prescalerTimer
	HAL_TIM_GenerateEvent(&prescalerTimer, TIM_EVENTSOURCE_UPDATE);
	HAL_TIM_Base_Start(&prescalerTimer);
#endif
	HAL_TIM_Base_Start_IT(&watchdogTimer);
}

inline void sw_watchdog_reset(void)
{
	watchdogTimer.Instance->CNT = 0;
}

#include "led.h"
void sw_watchdog_callback(void)
{
	uint16_t watchdogFlags = HAL_RTCEx_BKUPRead(NULL, BCKUP_REGISTER_WATCHDOG_FLAG);
	watchdogFlags |= (1<<0);
	HAL_RTCEx_BKUPWrite(NULL, BCKUP_REGISTER_WATCHDOG_FLAG, watchdogFlags);

	NVIC_SystemReset();
}

uint8_t sw_watchdog_isResetCause(void)
{
	uint16_t watchdogFlags = HAL_RTCEx_BKUPRead(NULL, BCKUP_REGISTER_WATCHDOG_FLAG);
	uint16_t watchdogResetFlag = watchdogFlags & (1 << 0);
	watchdogFlags &= ~(1<<0);
	HAL_RTCEx_BKUPWrite(NULL, BCKUP_REGISTER_WATCHDOG_FLAG, watchdogFlags);

	return watchdogResetFlag;
}
