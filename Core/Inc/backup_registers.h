/*
 * backup_registers.h
 *
 *  Created on: Mar 29, 2023
 *      Author: Karol
 */

#ifndef INC_BACKUP_REGISTERS_H_
#define INC_BACKUP_REGISTERS_H_

#include <stdint.h>

uint16_t BCKUP_getWkupCnt(void);
void BCKUP_setWkupCnt(uint16_t wakeUpCounter);


uint16_t BCKUP_getLastAlarm(void);
void BCKUP_setLastAlarm(uint16_t timeAlarm);


uint32_t BCKUP_getLastSyncTime(void);
void BCKUP_setLastSyncTime(uint32_t lastSyncTime);


uint16_t BCKUP_getWDFlag(void);
void BCKUP_setWDFlag(uint16_t watchdogFlag);

uint16_t BCKUP_getRetryCnt(void);
void BCKUP_setRetryCnt(uint16_t retryCnt);

#endif /* INC_BACKUP_REGISTERS_H_ */
