/*
 * backup_registers.c
 *
 *  Created on: 2 kwi 2023
 *      Author: Karol
 */

#include "backup_registers.h"
#include <stddef.h>
#include "rtc.h"

#define BCKUP_REGISTER_WKUP_CNT 		(1)
#define BCKUP_REGISTER_LAST_ALARM		(2)
#define BCKUP_REGISTER_LAST_SYNC_H		(3)
#define BCKUP_REGISTER_LAST_SYNC_L		(4)
#define BCKUP_REGISTER_RESTART_FLAG 	(5)

//[x, x, x, x, x, x, x, x, x, x, x, x, ER2, ER1, ER0, WD0]
#define WD_FLAG_POS 				(0)
#define WD_FLAG_MASK 				(1 << WD_FLAG_POS)
#define RETRY__CNT_POS 				(1)
#define RETRY__CNT_MASK 			(7 << RETRY__CNT_POS)

uint16_t BCKUP_getWkupCnt(void)
{
	return HAL_RTCEx_BKUPRead(NULL, BCKUP_REGISTER_WKUP_CNT);
}

void BCKUP_setWkupCnt(uint16_t wakeUpCounter)
{
	HAL_RTCEx_BKUPWrite(NULL, BCKUP_REGISTER_WKUP_CNT,wakeUpCounter);
}

uint16_t BCKUP_getLastAlarm(void)
{
	return HAL_RTCEx_BKUPRead(NULL, BCKUP_REGISTER_LAST_ALARM);
}

void BCKUP_setLastAlarm(uint16_t timeAlarm)
{
	HAL_RTCEx_BKUPWrite(NULL, BCKUP_REGISTER_LAST_ALARM, (timeAlarm & 0xFFFF));
}

uint32_t BCKUP_getLastSyncTime(void)
{
	uint32_t lastSyncTime = (HAL_RTCEx_BKUPRead(NULL, BCKUP_REGISTER_LAST_SYNC_H) << 16U);
	lastSyncTime = lastSyncTime | HAL_RTCEx_BKUPRead(NULL, BCKUP_REGISTER_LAST_SYNC_L);
	return lastSyncTime;
}

void BCKUP_setLastSyncTime(uint32_t lastSyncTime)
{
	HAL_RTCEx_BKUPWrite(NULL, BCKUP_REGISTER_LAST_SYNC_H, (lastSyncTime >> 16U));
	HAL_RTCEx_BKUPWrite(NULL, BCKUP_REGISTER_LAST_SYNC_L, (lastSyncTime & 0xFFFF));
}

uint16_t BCKUP_getWDFlag(void)
{
	return (HAL_RTCEx_BKUPRead(NULL, BCKUP_REGISTER_RESTART_FLAG) & WD_FLAG_MASK) >> WD_FLAG_POS;
}

void BCKUP_setWDFlag(uint16_t watchdogFlag)
{
	uint16_t flags = HAL_RTCEx_BKUPRead(NULL, BCKUP_REGISTER_RESTART_FLAG);
	if(watchdogFlag)
	{
		flags |= WD_FLAG_MASK;	//set
	}
	else
	{
		flags &= ~WD_FLAG_MASK;	//clear
	}
	HAL_RTCEx_BKUPWrite(NULL, BCKUP_REGISTER_RESTART_FLAG, flags);
}


uint16_t BCKUP_getRetryCnt(void)
{
	uint16_t flags = HAL_RTCEx_BKUPRead(NULL, BCKUP_REGISTER_RESTART_FLAG) & RETRY__CNT_MASK;
	return flags >> RETRY__CNT_POS;
}

void BCKUP_setRetryCnt(uint16_t retryCnt)
{
	uint16_t flags = HAL_RTCEx_BKUPRead(NULL, BCKUP_REGISTER_RESTART_FLAG);
	if(retryCnt > 7)
	{
		retryCnt = 7;
	}

	flags &= ~RETRY__CNT_MASK;
	flags |= retryCnt << RETRY__CNT_POS;
	HAL_RTCEx_BKUPWrite(NULL, BCKUP_REGISTER_RESTART_FLAG, flags);
}
