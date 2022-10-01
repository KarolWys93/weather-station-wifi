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

bool timeSync(uint8_t retries);
bool waitForConnection(uint8_t retries);

#endif /* APP_UTILS_H_ */
