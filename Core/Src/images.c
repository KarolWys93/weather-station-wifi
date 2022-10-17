/*
 * images.c
 *
 *  Created on: 01.05.2022
 *      Author: Karol
 */

#include "images.h"
#include "e-Paper/ImageData.h"

#include "e-Paper/EPD_1in54b.h"
#include "Config/DEV_Config.h"
#include "GUI/GUI_Paint.h"

#include <string.h>

void show_error_image(ERR_IMAGE errImg, char* errText)
{
	uint16_t textSize = 0;
	const unsigned char* image_buffer;
	uint8_t d_black[(EPD_WIDTH/8) * EPD_HEIGHT];

	switch (errImg) {
		case ERR_IMG_MEMORY_CARD:
			image_buffer = sd_err_b;
			break;
		case ERR_IMG_WIFI:
			image_buffer = no_wifi_b;
			break;
		default:
			image_buffer = error_b;
			break;
	}

	Paint_NewImage(d_black, EPD_WIDTH, EPD_HEIGHT, 0, WHITE);
	Paint_SelectImage(d_black);
	Paint_Clear(WHITE);
	Paint_DrawBitMap(image_buffer);

	if(errText != NULL)
	{
		sFONT* font = &Font20;

		textSize = strlen(errText)*(font->Width);
		if(textSize > 200)
		{
			font = &Font16;
			textSize = strlen(errText)*(font->Width);
		}

		if(textSize > 200) textSize = 200;
		textSize = (200-textSize)/2;
		Paint_DrawString(textSize, 140, errText, font, WHITE, BLACK);
	}

//	if(errText != NULL)
//	{
//		uint8_t line = 0;
//		sFONT* font = &Font16;
//		char* txtPtr = NULL;
//		uint16_t textLen = strlen(errText);
//
//
//		textSize = textLen*(font->Width);
//		if(textSize > 200)
//		{
//			txtPtr = strchr(errText, " ");
//
//
//			font = &Font16;
//			textSize = strlen(errText)*(font->Width);
//		}
//
//		if(textSize > 200) textSize = 200;
//		textSize = (200-textSize)/2;
//		Paint_DrawString(textSize, 140, errText, &Font20, WHITE, BLACK);
//	}

	EPD_Display(d_black, NULL);
}

void show_configMode_image(void)
{
	uint8_t d_black[(EPD_WIDTH/8) * EPD_HEIGHT];
	uint8_t d_grey[(EPD_WIDTH/8) * EPD_HEIGHT];

	Paint_NewImage(d_black, EPD_WIDTH, EPD_HEIGHT, 0, WHITE);
	Paint_NewImage(d_grey, EPD_WIDTH, EPD_HEIGHT, 0, WHITE);

	Paint_SelectImage(d_grey);
	Paint_Clear(WHITE);

	Paint_SelectImage(d_black);
	Paint_Clear(WHITE);

	Paint_DrawBitMap(settings_b);

	Paint_SelectImage(d_grey);
	Paint_Clear(WHITE);

	Paint_DrawBitMap(settings_g);

	EPD_SendBlackAndGrey(d_black, d_grey);
	EPD_Refresh();
}

void show_low_bat_image(void)
{
	uint8_t d_black[(EPD_WIDTH/8) * EPD_HEIGHT];
	uint8_t d_red[(EPD_WIDTH/8) * EPD_HEIGHT];
	//black & grey
	Paint_NewImage(d_black, EPD_WIDTH, EPD_HEIGHT, 0, WHITE);
	Paint_NewImage(d_red, EPD_WIDTH, EPD_HEIGHT, 0, WHITE);

	Paint_SelectImage(d_black);
	Paint_Clear(WHITE);
	Paint_SelectImage(d_red);
	Paint_Clear(WHITE);

	Paint_SelectImage(d_black);
	Paint_DrawRectangle(30, 30, 170, 100, BLACK, DRAW_FILL_EMPTY, DOT_PIXEL_6X6);
	Paint_DrawRectangle(170, 50, 180, 80, BLACK, DRAW_FILL_EMPTY, DOT_PIXEL_6X6);
	Paint_DrawRectangle(36, 36, 60, 94, RED, DRAW_FILL_FULL, DOT_PIXEL_1X1);

	Paint_DrawString(23, 140, "LOW BATTERY", &Font20, WHITE, BLACK);

	Paint_SelectImage(d_red);
	Paint_DrawRectangle(36, 36, 60, 94, RED, DRAW_FILL_FULL, DOT_PIXEL_1X1);
	Paint_DrawString(23, 140, "LOW BATTERY", &Font20, WHITE, BLACK);

	EPD_Display(d_black, d_red);

}
