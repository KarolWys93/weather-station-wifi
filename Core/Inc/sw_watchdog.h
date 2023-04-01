/*
 * sw_watchdog.h
 *
 *  Created on: Apr 1, 2023
 *      Author: Karol
 */

#ifndef INC_SW_WATCHDOG_H_
#define INC_SW_WATCHDOG_H_
#include <stdint.h>

void sw_watchdog_enable(uint16_t time_ms);
void sw_watchdog_reset(void);
uint8_t sw_watchdog_isResetCause(void);
void sw_watchdog_callback(void);

#endif /* INC_SW_WATCHDOG_H_ */
