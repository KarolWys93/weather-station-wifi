/*
 * button.h
 *
 *  Created on: Oct 23, 2022
 *      Author: Karol
 */

#ifndef INC_BUTTON_H_
#define INC_BUTTON_H_

#include <stdbool.h>

bool button_isWakeUpPressed(void);
bool button_isConfigModePressed(void);
bool button_isFactoryResetPressed(void);

#endif /* INC_BUTTON_H_ */
