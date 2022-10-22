/*
 * button.c
 *
 *  Created on: Oct 23, 2022
 *      Author: Karol
 */

#include "button.h"
#include "main.h"

bool button_isWakeUpPressed(void)
{
	return (GPIO_PIN_SET == HAL_GPIO_ReadPin(SYS_WKUP_GPIO_Port, SYS_WKUP_Pin));
}

bool button_isConfigModePressed(void)
{
	return (GPIO_PIN_RESET == HAL_GPIO_ReadPin(CONFIG_MODE_GPIO_Port, CONFIG_MODE_Pin));
}

bool button_isFactoryResetPressed(void)
{
	return (GPIO_PIN_RESET == HAL_GPIO_ReadPin(FACTORY_RST_GPIO_Port, FACTORY_RST_Pin));
}
