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

#include "fatfs.h"
#include "system_info.h"
#include "system_power.h"
#include <stdio.h>


typedef enum ImageName
{
	MEMORY_ERR_IMG = 0,
	FORECAST_BG_IMG = 1,
	CONFIG_MODE_IMG = 2,
	LOW_BAT_IMG = 3,
	WIFI_ERROR_IMG = 4,
	GENERAL_ERROR_IMG = 5
} ImageName;

typedef enum BufferType
{
	BUFFER_BLACK = 0,
	BUFFER_GREY,
	BUFFER_RED
} BufferType;

static uint8_t loadFromMemory(ImageName imgName, BufferType bufferType);
static void img_loadBlackGrey(ImageName imgName, uint8_t* blackBuff, uint8_t* greyBuff);
static void img_loadRed(ImageName imgName, uint8_t* redBuff);

static uint8_t readImgFile(char* filePath, uint8_t* buffer, uint32_t size);
static uint8_t saveImgFile(char* filePath, uint8_t* buffer, uint32_t size);


static uint8_t loadFromMemory(ImageName imgName, BufferType bufferType)
{
	char imgPath[16];
	switch (bufferType) {
		case BUFFER_BLACK:
			sprintf(imgPath, "%s/%u_b.epd", DIR_PATH_IMG, imgName);
			break;
		case BUFFER_GREY:
			sprintf(imgPath, "%s/%u_g.epd", DIR_PATH_IMG, imgName);
			break;
		case BUFFER_RED:
			sprintf(imgPath, "%s/%u_r.epd", DIR_PATH_IMG, imgName);
			break;
		default:
			return 1;
	}

	return readImgFile(imgPath, Paint.Image, Paint.WidthByte*Paint.HeightByte);
}

static void img_loadBlackGrey(ImageName imgName, uint8_t* blackBuff, uint8_t* greyBuff)
{
	switch (imgName) {
	    case MEMORY_ERR_IMG:
	    	Paint_SelectImage(greyBuff);
	    	Paint_Clear(WHITE);
	    	Paint_SelectImage(blackBuff);
	    	Paint_DrawBitMap(sd_err_b);
	    	break;

	    case FORECAST_BG_IMG:
	    	Paint_SelectImage(greyBuff);
	    	Paint_Clear(WHITE);
	    	Paint_SelectImage(blackBuff);
	    	Paint_DrawBitMap(forecast_black);
	    	break;

		default:
			Paint_SelectImage(greyBuff);
			if(loadFromMemory(imgName, BUFFER_GREY))
			{
				Paint_Clear(WHITE);
			}

			Paint_SelectImage(blackBuff);
			if(loadFromMemory(imgName, BUFFER_BLACK))
			{
				Paint_Clear(WHITE);
			}
	}
}

static void img_loadRed(ImageName imgName, uint8_t* redBuff)
{
	switch (imgName) {
	    case MEMORY_ERR_IMG:
	    	Paint_SelectImage(redBuff);
	    	Paint_Clear(WHITE);
	    	break;

		case FORECAST_BG_IMG:
			Paint_SelectImage(redBuff);
			Paint_DrawBitMap(forecast_red);
			break;

		default:
			Paint_SelectImage(redBuff);
			if(loadFromMemory(imgName, BUFFER_RED))
			{
				Paint_Clear(WHITE);
			}
	}
}

static uint8_t readImgFile(char* filePath, uint8_t* buffer, uint32_t size)
{
	FIL file;
	UINT readBytes = 0;

	if(FR_OK != f_open(&file, filePath, FA_READ))
	{
		return 1;
	}

	f_read(&file, buffer, size, &readBytes);
	f_close(&file);

	if(size != readBytes)
	{
		return 2;
	}
	else
	{
		return 0;
	}
}

static uint8_t saveImgFile(char* filePath, uint8_t* buffer, uint32_t size)
{
	FIL file;
	UINT writeBytes = 0;

	if(FR_OK != f_open(&file, filePath, FA_CREATE_ALWAYS | FA_WRITE))
	{
		return 1;
	}

	f_write(&file, buffer, size, &writeBytes);
	f_close(&file);

	if(size != writeBytes)
	{
		return 2;
	}
	else
	{
		return 0;
	}
}

void load_forecastImg_BlackGrey(uint8_t* black_buff, uint8_t* grey_buff)
{
	img_loadBlackGrey(FORECAST_BG_IMG, black_buff, grey_buff);
}

void load_forecastImg_Red(uint8_t* red_buff)
{
	img_loadRed(FORECAST_BG_IMG, red_buff);
}

void show_emptyForecast_image(void)
{
    uint8_t d_black[(EPD_WIDTH/8) * EPD_HEIGHT];
    uint8_t d_grey[(EPD_WIDTH/8) * EPD_HEIGHT];
    uint8_t *d_red = d_black;

    Paint_NewImage(d_black, EPD_WIDTH, EPD_HEIGHT, ROTATE_270, WHITE);
    Paint_NewImage(d_grey, EPD_WIDTH, EPD_HEIGHT, ROTATE_270, WHITE);

    load_forecastImg_BlackGrey(d_black, d_grey);
    EPD_SendBlackAndGrey(d_black, d_grey);

    load_forecastImg_Red(d_red);
    EPD_SendRed(d_red);

    EPD_Refresh();
}

void save_img_BlackGrey(uint8_t* black_buff, uint8_t* grey_buff)
{
    #ifdef DEBUG
	f_mkdir(DIR_PATH_SCREEN_DUMP);
	if(NULL != black_buff)
	{
		saveImgFile(DIR_PATH_SCREEN_DUMP"/b.epd", black_buff, Paint.WidthByte*Paint.HeightByte);
	}
	else
	{
		f_unlink(DIR_PATH_SCREEN_DUMP"/b.epd");
	}

	if(NULL != grey_buff)
	{
		saveImgFile(DIR_PATH_SCREEN_DUMP"/g.epd", grey_buff, Paint.WidthByte*Paint.HeightByte);
	}
	else
	{
		f_unlink(DIR_PATH_SCREEN_DUMP"/g.epd");
	}
	#endif
}

void save_img_Red(uint8_t* red_buff)
{
    #ifdef DEBUG
	f_mkdir(DIR_PATH_SCREEN_DUMP);
	if(NULL != red_buff)
	{
		saveImgFile(DIR_PATH_SCREEN_DUMP"/r.epd", red_buff, Paint.WidthByte*Paint.HeightByte);
	}
	else
	{
		f_unlink(DIR_PATH_SCREEN_DUMP"/r.epd");
	}
    #endif
}

void show_error_image(ERR_IMAGE errImg, char* errText)
{
	uint16_t textSize = 0;
	ImageName imageName;

	uint8_t d_black[(EPD_WIDTH/8) * EPD_HEIGHT];
	uint8_t d_grey[(EPD_WIDTH/8) * EPD_HEIGHT];
	uint8_t *d_red = d_black;

	uint8_t isCharging = system_powerStatus();
	uint8_t batteryLvl = system_batteryLevel();

	Paint_NewImage(d_grey, EPD_WIDTH, EPD_HEIGHT, ROTATE_270, WHITE);
	Paint_NewImage(d_black, EPD_WIDTH, EPD_HEIGHT, ROTATE_270, WHITE);

	switch (errImg)
	{
	    case ERR_IMG_MEMORY_CARD:
		    imageName = MEMORY_ERR_IMG;
		    break;
		case ERR_IMG_WIFI:
			imageName = WIFI_ERROR_IMG;
			break;
		default:
			imageName = GENERAL_ERROR_IMG;
			break;
	}

	img_loadBlackGrey(imageName, d_black, d_grey);

	Paint_SelectImage(d_black);
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

	draw_battery_level_black(d_black, batteryLvl, isCharging);

	EPD_SendBlackAndGrey(d_black, d_grey);

	Paint_SelectImage(d_red);
	img_loadRed(imageName, d_red);

	draw_battery_level_black(d_red, batteryLvl, isCharging);
	EPD_SendRed(d_red);

	EPD_Refresh();
}

void show_configMode_image(void)
{
	uint8_t d_black[(EPD_WIDTH/8) * EPD_HEIGHT];
	uint8_t d_grey[(EPD_WIDTH/8) * EPD_HEIGHT];
	uint8_t *d_red = d_black;

	Paint_NewImage(d_black, EPD_WIDTH, EPD_HEIGHT, ROTATE_270, WHITE);
	Paint_NewImage(d_grey, EPD_WIDTH, EPD_HEIGHT, ROTATE_270, WHITE);

	img_loadBlackGrey(CONFIG_MODE_IMG, d_black, d_grey);
	EPD_SendBlackAndGrey(d_black, d_grey);

	img_loadRed(CONFIG_MODE_IMG, d_red);
	EPD_SendRed(d_red);

	EPD_Refresh();
}

void show_low_bat_image(void)
{
	uint8_t d_black[(EPD_WIDTH/8) * EPD_HEIGHT];
	uint8_t d_grey[(EPD_WIDTH/8) * EPD_HEIGHT];
	uint8_t *d_red = d_black;

	Paint_NewImage(d_black, EPD_WIDTH, EPD_HEIGHT, ROTATE_270, WHITE);
	Paint_NewImage(d_grey, EPD_WIDTH, EPD_HEIGHT, ROTATE_270, WHITE);

	img_loadBlackGrey(LOW_BAT_IMG, d_black, d_grey);
	Paint_SelectImage(d_black);
	Paint_DrawString(23, 140, "LOW BATTERY", &Font20, WHITE, BLACK);

	EPD_SendBlackAndGrey(d_black, d_grey);

	img_loadRed(LOW_BAT_IMG, d_red);
	Paint_SelectImage(d_red);
	Paint_DrawString(23, 140, "LOW BATTERY", &Font20, WHITE, RED);
	EPD_SendRed(d_red);

	EPD_Refresh();
}

void draw_battery_level_black(uint8_t* black_buff, uint8_t batteryLvl, uint8_t isCharging)
{
	if(batteryLvl > 100) batteryLvl = 100;
	Paint_SelectImage(black_buff);

	Paint_DrawRectangle(1, 1, 200, 2, WHITE, DRAW_FILL_FULL, DOT_PIXEL_1X1);
	if(!isCharging)
	{
		Paint_DrawRectangle(1, 1, batteryLvl*2, 2, BLACK, DRAW_FILL_FULL, DOT_PIXEL_1X1);
	}
	else
	{
		Paint_DrawRectangle(1, 1, 200, 2, BLACK, DRAW_FILL_FULL, DOT_PIXEL_1X1);
	}
}

void draw_battery_level_red(uint8_t* red_buff, uint8_t batteryLvl, uint8_t isCharging)
{
	(void) batteryLvl;
	Paint_SelectImage(red_buff);
	Paint_DrawRectangle(1, 1, 200, 2, WHITE, DRAW_FILL_FULL, DOT_PIXEL_1X1);
	if(isCharging)
	{
		Paint_DrawRectangle(1, 1, 200, 2, RED, DRAW_FILL_FULL, DOT_PIXEL_1X1);
	}
}
