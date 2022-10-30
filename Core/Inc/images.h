/*
 * images.h
 *
 *  Created on: 01.05.2022
 *      Author: Karol
 */

#ifndef IMAGES_H_
#define IMAGES_H_

#include <stdint.h>

typedef enum ERR_IMAGE
{
	ERR_IMG_GENERAL = 0,
	ERR_IMG_MEMORY_CARD,
	ERR_IMG_WIFI
} ERR_IMAGE;

void load_forecastImg_BlackGrey(uint8_t* black_buff, uint8_t* grey_buff);
void load_forecastImg_Red(uint8_t* red_buff);

void save_img_BlackGrey(uint8_t* black_buff, uint8_t* grey_buff);
void save_img_Red(uint8_t* red_buff);

void show_error_image(ERR_IMAGE errImg, char* errText);
void show_configMode_image(void);
void show_low_bat_image(void);

#endif /* IMAGES_H_ */
