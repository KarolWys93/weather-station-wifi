/*
 * images.h
 *
 *  Created on: 01.05.2022
 *      Author: Karol
 */

#ifndef IMAGES_H_
#define IMAGES_H_

typedef enum ERR_IMAGE
{
	ERR_IMG_GENERAL = 0,
	ERR_IMG_MEMORY_CARD,
	ERR_IMG_WIFI
} ERR_IMAGE;

void show_error_image(ERR_IMAGE errImg, char* errText);
void show_configMode_image(void);

#endif /* IMAGES_H_ */
