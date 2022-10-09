/*
 * utils.h
 *
 *  Created on: 01.05.2022
 *      Author: Karol
 */

#ifndef APP_UTILS_H_
#define APP_UTILS_H_

#include <stdint.h>
#include <stdbool.h>

#define OLD_TIME_STAMP 946681200	//00:00:00 01-01-2000

bool timeSync(uint8_t retries);
bool waitForConnection(uint8_t retries);

#endif /* APP_UTILS_H_ */
