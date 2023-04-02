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

#define WD_FREEZE_UNDER_DEBUG_SESSION() __HAL_DBGMCU_FREEZE_TIM4()

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

#ifdef DEBUG
	WD_FREEZE_UNDER_DEBUG_SESSION();
#endif

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

void sw_watchdog_callback(void)
{
	BCKUP_setWDFlag(1);
	NVIC_SystemReset();
}

uint8_t sw_watchdog_isResetCause(void)
{
	uint16_t watchdogResetFlag = BCKUP_getWDFlag();
	BCKUP_setWDFlag(0);
	return watchdogResetFlag;
}
