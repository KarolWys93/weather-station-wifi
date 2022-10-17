/*
 * httpServer_internal.h
 *
 *  Created on: 14.02.2021
 *      Author: Karol
 */

#ifndef HTTPSERVER_INTERNAL_H_
#define HTTPSERVER_INTERNAL_H_

#include <stdint.h>
#include "fatfs.h"

typedef enum HTTP_STATUS
{
	HTTP_SERVER_OK = 0,
	HTTP_SERVER_NOT_FOUND,
	HTTP_SERVER_BAD_REQUEST,
	HTTP_SERVER_ERROR,
	HTTP_SERVER_CRITICAL_ERROR
} HTTP_STATUS;

int32_t httpServer_sendData(char* data, uint16_t data_size);
int32_t httpServer_sendFile(FIL* file, char* buffer, const uint32_t bufferSize, const uint16_t headerSize, uint32_t data_size);

#endif /* HTTPSERVER_INTERNAL_H_ */
