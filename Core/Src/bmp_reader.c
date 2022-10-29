/*
 * bmp_reader.c
 *
 *  Created on: Oct 29, 2022
 *      Author: Karol
 */
#include "bmp_reader.h"
#include "GUI/GUI_Paint.h"
#include "fatfs.h"

uint8_t readBmpFile(char* filePath)	//TODO: there should be pointer to Paint object
{
	FIL file;
	UINT readBytes = 0;

	if(FR_OK != f_open(&file, filePath, FA_READ))
	{
		return 1;
	}

	f_read(&file, Paint.Image, Paint.HeightByte*Paint.WidthByte, &readBytes);

	if(Paint.HeightByte*Paint.WidthByte != readBytes)
	{
		return 2;
	}

	f_close(&file);
	return 0;
}
