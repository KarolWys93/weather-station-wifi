/*
 * httpServerApp.c
 *
 *  Created on: 24.01.2021
 *      Author: Karol
 */

#include "httpServerApp.h"

#include "http_requests.h"
#include "httpServer_api.h"
#include "httpServer_internal.h"

#include "../../wifi/wifi_esp.h"
#include "fatfs.h"

#include "button.h"

#include "logger.h"
#include "utils.h"

#include "images.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#define HTTP_SERVER_STATIC_PATH_LEN   (sizeof(HTTP_SERVER_STATIC_PATH))
#define HTTP_SERVER_REQUES_PATH_SIZE  (HTTP_SERVER_REQUES_PATH_MAX_SIZE-HTTP_SERVER_STATIC_PATH_LEN)

static bool g_serverRun;
static uint8_t g_linkID;

static int8_t handleRequest(char* requestPtr, uint16_t requestSize);
static HTTP_STATUS staticFileRequest(char* requestPtr, uint16_t requestSize, char* path);
static HTTP_STATUS errorResponse(uint16_t errorCode);
static HTTP_STATUS mrSandMan(uint8_t fast);

static int32_t httpServer_sendFile2(FIL* file, char* buffer, const uint32_t bufferSize, const uint16_t headerSize, uint32_t data_size);
static int32_t httpServer_sendData2(char* data, uint16_t data_size);

static HTTP_STATUS mrSandMan(uint8_t fast)
{
	const char* const popFile = "/html/vid/sandman.webm";
	FIL file;
	FILINFO fileInfo;
	uint32_t fileSize = 0;
	uint32_t headerSize = 0;
	char responseBuffer[HTTP_SERVER_RESPONSE_SIZE];

	memset(&fileInfo, 0, sizeof(FILINFO));

	if(FR_OK == f_stat(popFile, &fileInfo))
	{
		fileSize = fileInfo.fsize;
	}

	headerSize = HTTP_createResponseHeader(responseBuffer, sizeof(responseBuffer), 200, fileSize);
	if(headerSize < 0)
	{
		return HTTP_SERVER_NOT_FOUND;
	}

	if(fileSize > 0 && FR_OK == f_open(&file, popFile, FA_READ))
	{
		if(fast == 0)
		{
			httpServer_sendFile(&file, responseBuffer, sizeof(responseBuffer), headerSize, fileSize);
		}
		else
		{
			httpServer_sendFile2(&file, responseBuffer, sizeof(responseBuffer), headerSize, fileSize);
		}
	}
	f_close(&file);
	return HTTP_SERVER_OK;
}

static HTTP_STATUS errorResponse(uint16_t errorCode)
{
	FIL file;
	FILINFO fileInfo;
	uint32_t fileSize;
	uint32_t headerSize;
	char filePath[HTTP_SERVER_STATIC_PATH_LEN + 16];	// /html/XXX.html\0
	char responseBuffer[256];

	Logger(LOG_WRN, "Response %d", errorCode);

	memset(&fileInfo, 0, sizeof(FILINFO));

	sprintf(filePath, "%s/%d.html", HTTP_SERVER_STATIC_PATH, errorCode);

	if(FR_OK != f_stat(filePath, &fileInfo))
	{
		fileSize = 0;
	}
	else
	{
		fileSize = fileInfo.fsize;
	}

	headerSize = HTTP_createResponseHeader(responseBuffer, sizeof(responseBuffer), errorCode, fileSize);
	if(headerSize < 0)
	{
		return HTTP_SERVER_CRITICAL_ERROR;
	}
	if(0 > (headerSize = HTTP_addHeaderField(responseBuffer, sizeof(responseBuffer), "Content-Encoding", "gzip")))
	{
		return HTTP_SERVER_ERROR;
	}

	if(fileSize > 0)
	{
		if(FR_OK != f_open(&file, filePath, FA_READ))
		{
			return HTTP_SERVER_ERROR;
		}

		httpServer_sendFile(&file, responseBuffer, sizeof(responseBuffer), headerSize, fileSize);
		f_close(&file);
	}
	else
	{
		httpServer_sendData(responseBuffer, headerSize);
	}

	return HTTP_SERVER_OK;
}

static HTTP_STATUS staticFileRequest(char* requestPtr, uint16_t requestSize, char* path)
{
	FIL file;
	FILINFO fileInfo;
	uint32_t headerSize;
	uint32_t fileSize = 0;
	char responseBuffer[HTTP_SERVER_RESPONSE_SIZE];

	char filePath[HTTP_SERVER_REQUEST_PATH_MAX_SIZE + HTTP_SERVER_STATIC_PATH_LEN];
	char* payloadPtr;

	memset(&fileInfo, 0, sizeof(FILINFO));

	if(0 == strcmp("/", path))
	{
		sprintf(filePath, "%s/index.html", HTTP_SERVER_STATIC_PATH);
	}
	else
	{
		sprintf(filePath, "%s%s", HTTP_SERVER_STATIC_PATH, path);
	}

	if(FR_OK != f_stat(filePath, &fileInfo))
	{
		return HTTP_SERVER_NOT_FOUND;
	}
	fileSize = fileInfo.fsize;

	if(FR_OK != f_open(&file, filePath, FA_READ))
	{
		return HTTP_SERVER_ERROR;
	}

	if(0 > (headerSize = HTTP_createResponseHeader(responseBuffer, HTTP_SERVER_RESPONSE_SIZE, 200, fileSize)))
	{
		f_close(&file);
		return HTTP_SERVER_ERROR;
	}

	if(0 > (headerSize = HTTP_addHeaderField(responseBuffer, HTTP_SERVER_RESPONSE_SIZE, "Cache-Control", "public, max-age=604800, immutable")))
	{
		f_close(&file);
		return HTTP_SERVER_ERROR;
	}

	if(0 > (headerSize = HTTP_addHeaderField(responseBuffer, HTTP_SERVER_RESPONSE_SIZE, "Content-Encoding", "gzip")))
	{
		f_close(&file);
		return HTTP_SERVER_ERROR;
	}

	payloadPtr = HTTP_getContent(responseBuffer, HTTP_SERVER_RESPONSE_SIZE);
	if(payloadPtr == NULL) {
		f_close(&file);
		return HTTP_SERVER_ERROR;
	}


	httpServer_sendFile(&file, responseBuffer, sizeof(responseBuffer), headerSize, fileSize);

	f_close(&file);
	return HTTP_SERVER_OK;
}

static int8_t handleRequest(char* requestPtr, uint16_t requestSize)
{


	HTTP_STATUS status = 0;
	char path[HTTP_SERVER_REQUEST_PATH_MAX_SIZE];
	HTTP_METHOD requestMethod = HTTP_getMethod(requestPtr, requestSize);
	if(HTTP_UNDEF == requestMethod)
	{
		return errorResponse(501);	//501 Not Implemented
	}

	if(0 > HTTP_getPath(requestPtr, requestSize, path, HTTP_SERVER_REQUEST_PATH_MAX_SIZE))
	{
		return errorResponse(400); //400 Bad Request
	}

	Logger(LOG_INF, "Request %s: %s", (requestMethod == HTTP_GET ? "GET" : "POST"), path);

	/* API endpoints */
	if(HTTP_GET == requestMethod)
	{
		if(0 == strcmp(path, "/api/status"))
		{
			status = serverAPI_status(requestPtr, requestSize);
		}
		else if(0 == strcmp(path, "/api/ap_conf"))
		{
			status = serverAPI_getAPconfig(requestPtr, requestSize);
		}
		else if(0 == strcmp(path, "/api/ap_list"))
		{
			status = serverAPI_getListOfStations(requestPtr, requestSize);
		}
		else if(0 == strcmp(path, "/api/wifi_conf"))
		{
			status = serverAPI_getWiFiconfig(requestPtr, requestSize);
		}
		else if(0 == strcmp(path, "/api/fcast_conf"))
		{
			status = serverAPI_getForecastConfig(requestPtr, requestSize);
		}
		else if(0 == strcmp(path, "/api/system_conf"))
		{
			status = serverAPI_getSystemConfig(requestPtr, requestSize);
		}
		else if(0 == strcmp(path, "/api/restart"))
		{
			status = serverAPI_restartDevice(requestPtr, requestSize, 0);
		}
		else if(0 == strcmp(path, "/api/restart_conf"))
		{
			status = serverAPI_restartDevice(requestPtr, requestSize, 1);
		}
		else if(0 == strcmp(path, "/api/restore_config"))
		{
			status = serverAPI_restoreDefaultSettings(requestPtr, requestSize);
		}
		else if(0 == strcmp(path, "/api/logs"))
		{
			status = serverAPI_logs(requestPtr, requestSize);
		}
		else if(0 == strcmp(path, "/api/voltage"))
		{
			status = serverAPI_voltage(requestPtr, requestSize);
		}
		else if(0 == strcmp(path, "/pop"))
		{
			status = mrSandMan(0);
		}
		else if(0 == strcmp(path, "/pop2"))
		{
			status = mrSandMan(1);
		}
		else
		{
			status = staticFileRequest(requestPtr, requestSize, path);
		}
	}
	else if(HTTP_POST == requestMethod)
	{
		if(0 == strcmp(path, "/api/ap_conf"))
		{
			status = serverAPI_setAPconfig(requestPtr, requestSize);
		}
		else if(0 == strcmp(path, "/api/wifi_conf"))
		{
			status = serverAPI_setWiFiconfig(requestPtr, requestSize);
		}
		else if(0 == strcmp(path, "/api/fcast_conf"))
		{
			status = serverAPI_setForecastConfig(requestPtr, requestSize);
		}
		else if(0 == strcmp(path, "/api/device_name"))
		{
			status = serverAPI_setHostname(requestPtr, requestSize);
		}
		else if(0 == strcmp(path, "/api/led_ind"))
		{
			status = serverAPI_setLedIndicator(requestPtr, requestSize);
		}
		else
		{
			status = HTTP_SERVER_NOT_FOUND;
		}
	}

	if(HTTP_SERVER_OK != status)	//handle errors
	{
		switch (status) {
		case HTTP_SERVER_NOT_FOUND:
			status = errorResponse(404); //not found
			break;
		case HTTP_SERVER_ERROR:
			status = errorResponse(500); //internal server error
			break;
		case HTTP_SERVER_BAD_REQUEST:
			status = errorResponse(400); //bad request
			break;
		default:
			status = HTTP_SERVER_CRITICAL_ERROR;
			break;
		}
	}

	return status;
}


int32_t httpServer_sendData(char* data, uint16_t data_size)
{
	if(WIFI_RESP_OK == WiFi_SendData(g_linkID, (uint8_t*)data, data_size))
	{
		return data_size;
	}
	else
	{
		return -1;
	}
}

static int32_t httpServer_sendData2(char* data, uint16_t data_size)
{
	if(WIFI_RESP_OK == WiFi_SendDataEx(g_linkID, (uint8_t*)data, data_size, false))
	{
		return data_size;
	}
	else
	{
		return -1;
	}
}

static int32_t httpServer_sendFile2(FIL* file, char* buffer, const uint32_t bufferSize, const uint16_t headerSize, uint32_t data_size)
{
	char* payloadPtr = buffer + headerSize;
	uint32_t dataLeft = data_size;
	uint32_t dataToRead;
	uint32_t dataToSend = headerSize;
	uint32_t dataReadFormFile = 0;

	char buffer2[bufferSize];
	uint8_t bufferIndex = 0;
	char* bufferTab[2] = {buffer, buffer2};

	char* activeBuffer = bufferTab[bufferIndex];

	bool firstTime = true;

	while(dataLeft > 0)
	{
		dataToRead = dataLeft;
		if(dataToSend + dataToRead > bufferSize)
		{
			dataToRead = bufferSize - dataToSend;
		}

		if((FR_OK != f_read(file, (void*)payloadPtr, dataToRead, (UINT*)&dataReadFormFile)) || (dataReadFormFile != dataToRead))
		{
			return dataLeft;
		}
		dataToSend += dataReadFormFile;

		if(!firstTime)
		{
			WiFi_SendingComplete();
		}
		firstTime = false;

		if(0 > httpServer_sendData2(activeBuffer, dataToSend))
		{
			return dataLeft;
		}

		dataLeft-=dataReadFormFile;
		dataToSend = 0;
		bufferIndex++;
		activeBuffer = bufferTab[bufferIndex%2];
		payloadPtr = activeBuffer;
	}
	return dataLeft;
}

int32_t httpServer_sendFile(FIL* file, char* buffer, const uint32_t bufferSize, const uint16_t headerSize, uint32_t data_size)
{
	char* payloadPtr = buffer + headerSize;
	uint32_t dataLeft = data_size;
	uint32_t dataToRead;
	uint32_t dataToSend = headerSize;
	uint32_t dataReadFormFile = 0;

	while(dataLeft > 0)
	{
		dataToRead = dataLeft;
		if(dataToSend + dataToRead > bufferSize)
		{
			dataToRead = bufferSize - dataToSend;
		}

		if((FR_OK != f_read(file, (void*)payloadPtr, dataToRead, (UINT*)&dataReadFormFile)) || (dataReadFormFile != dataToRead))
		{
			return dataLeft;
		}
		dataToSend += dataReadFormFile;
		if(0 > httpServer_sendData(buffer, dataToSend))
		{
			return dataLeft;
		}
		dataLeft-=dataReadFormFile;
		dataToSend = 0;
		payloadPtr = buffer;
	}
	return dataLeft;
}

uint8_t runServerApp(uint16_t port, uint8_t maxConnection, uint16_t serverTimeout)
{
	char requestBuffer[HTTP_SERVER_REQUEST_SIZE];
	uint32_t secondCounter = 0;
	uint32_t tickTime = 0;
	uint32_t lastActivityTick = 0;
	int8_t status = 0;
	bool timeIsSynced = false;
	uint32_t exitButtonTime = 0;

	g_linkID = 0;


	if(WIFI_RESP_OK != WiFi_OpenServerSocket(port, maxConnection, serverTimeout, 1000))
	{
		Logger(LOG_ERR, "Cannot open server socket");
		return 1;
	}

	if(!WiFi_MessageReceivingIsRunning())
	{
		if(WIFI_RESP_OK != WiFi_MessageReceivingStart(true))
		{
			return 2;
		}
	}

	lastActivityTick = HAL_GetTick();
	g_serverRun = true;
	while(g_serverRun)
	{
		//every 1 second
		if(HAL_GetTick() - tickTime > 1000)
		{
			secondCounter++;

			Logger_sync();

			WiFi_UpdateStatus(2000);
			tickTime = HAL_GetTick();

			if(!timeIsSynced && wifiStatus.connectedToAP)
			{
				timeIsSynced = timeSync(0);
			}

			if(button_isWakeUpPressed())
			{
				if(exitButtonTime++ > 5) system_restart(0);
			}
			else
			{
				exitButtonTime = 0;
			}


			//every 10 second
			if(secondCounter % 10 == 0)
			{
				//power check
				if(system_batteryLevel() == 0 && !system_isCharging())
				{
					show_low_bat_image();
					status = 0;
					break;
				}
			}
	    }

		volatile SWiFiLinkStatus* linkStatus = &wifiStatus.linksStatus[g_linkID];

		if(linkStatus->connected && (linkStatus->dataWaiting > 0))
		{
			int16_t rcvLen = WiFi_ReadData(g_linkID, (uint8_t*) requestBuffer, linkStatus->dataWaiting, 1000);
			Logger(LOG_INF, "Link %d received %d bytes", g_linkID, rcvLen);
			if(rcvLen > 0)
			{
				HTTP_STATUS reqStatus = handleRequest(requestBuffer, rcvLen);
				Logger(LOG_INF, "Link %d reqStatus: %d", g_linkID, reqStatus);

				if(HTTP_SERVER_OK != reqStatus)
				{
					status = 3;
					break;
				}

				lastActivityTick = HAL_GetTick();

			}
//			if(linkStatus->connected)
//			{
//				WiFi_CloseConnection(g_linkID);
//			}
		}

		g_linkID = (g_linkID+1)%5;

		if(HAL_GetTick() - lastActivityTick > HTTP_SERVER_INACTIVITY_TIMEOUT)
		{
			Logger(LOG_INF, "Server shutdown due to inactivity");
			system_restart(0);
		}

		HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
	}

	WiFi_CloseConnection(5);	//close all connections
	WiFi_CloseServerSocket(port);

	return status;
}

