/*
 * system_power.h
 *
 *  Created on: 21 mar 2023
 *      Author: Karol
 */

#ifndef INC_SYSTEM_POWER_H_
#define INC_SYSTEM_POWER_H_
#include <stdint.h>

typedef enum
{
	SYSTEM_POWER_BATTERY = 0,
	SYSTEM_POWER_DC_ADAPTER,
	SYSTEM_POWER_CHARGING
} SYSTEM_POWER_STATUS;

void system_powerInit(void);
uint8_t system_isCharging(void);
SYSTEM_POWER_STATUS system_powerStatus(void);
uint32_t system_batteryVoltage(void);
uint8_t system_batteryLevel(void);


#endif /* INC_SYSTEM_POWER_H_ */
