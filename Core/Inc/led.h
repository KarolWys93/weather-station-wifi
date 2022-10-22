/*
 * led.h
 *
 *  Created on: Oct 22, 2022
 *      Author: Karol
 */

#ifndef INC_LED_H_
#define INC_LED_H_

typedef enum LED_COLOR
{
	LED_OFF = 0,
	LED_RED,
	LED_GREEN,
	LED_YELLOW,
	LED_BLUE,
	LED_MAGENTA,
	LED_CYAN,
	LED_WHITE
} LED_COLOR;

void led_setColor(LED_COLOR color);

#endif /* INC_LED_H_ */
