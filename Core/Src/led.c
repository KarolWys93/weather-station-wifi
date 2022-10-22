/*
 * led.c
 *
 *  Created on: Oct 22, 2022
 *      Author: Karol
 */

#include "led.h"
#include "system.h"
#include "main.h"

void led_setColor(LED_COLOR color)
{
	if(!system_isLedIndicatorOn())
	{
		color = LED_OFF;
	}
	GPIO_PinState r = !(color & 0b001);
	GPIO_PinState g = !(color & 0b010);
	GPIO_PinState b = !(color & 0b100);

	HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, r);
	HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, g);
	HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, b);
}
