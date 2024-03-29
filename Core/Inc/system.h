/*
 * system.h
 *
 *  Created on: 27.04.2022
 *      Author: Karol
 */

#ifndef SYSTEM_H_
#define SYSTEM_H_

#include <stdint.h>
#include "system_info.h"
#include "system_power.h"


//----FUNCTIONS-----

void system_init(void);
void system_shutdown(void);
void system_restart(uint8_t configMode);

void system_setWakeUpTimer(uint32_t seconds);

char* const system_getHostName(void);
uint8_t system_isLedIndicatorOn(void);
uint8_t system_isConfigModeOn(void);

void system_sleep(uint32_t miliseconds);

uint8_t system_restoreDefault(void);

#endif /* SYSTEM_H_ */
