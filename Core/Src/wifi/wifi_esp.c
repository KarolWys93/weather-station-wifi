/*
 * wifi_esp.c
 *
 *  Created on: 11.01.2021
 *      Author: Karol
 */

#include "wifi_esp.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "dma.h"


#define WIFI_MESSAGE_BUFFER_SIZE 64

volatile SWiFiStatus wifiStatus;

static char espMsgBuff[WIFI_MESSAGE_BUFFER_SIZE];
static volatile bool g_newMessage = false;
static volatile bool g_handlerLoopRunning = false;

static volatile bool sendingComplete = true;
static volatile bool asyncSend = false;

static inline void restoreHandlerLoop(bool restore)
{
	if(restore)
	{
		WiFi_MessageReceivingStart(true);
	}
}

static inline void waitForMessageAndStop(void)
{
	if(!WiFi_MessageReceivingIsRunning())
	{
		WiFi_MessageReceivingStart(false);
	}
	else
	{
		g_newMessage = false;
		g_handlerLoopRunning = false;
	}
}

inline static inline char* getMessage(void)
{
	g_newMessage = false;
	return (char*) espMsgBuff;
}

inline static bool messageReceived(void)
{
	return g_newMessage;
}

static Wifi_RespStatus waitForResp(const uint32_t timeout)
{
	Wifi_RespStatus status = WIFI_RESP_TIMEOUT;

	bool loopProcTmp = WiFi_MessageReceivingIsRunning();
	uint32_t startTime = HAL_GetTick();

	waitForMessageAndStop();

	do
	{
		if(messageReceived())
		{
			if(0 == strcmp(getMessage(), "OK"))
			{
				status = WIFI_RESP_OK;
				break;
			}
			else if(0 == strcmp(getMessage(), "ERROR"))
			{
				status = WIFI_RESP_ERROR;
				break;
			}
			else
			{
				waitForMessageAndStop();
			}
		}
		else
		{
			HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
		}
	}
	while(startTime + timeout > HAL_GetTick());

	restoreHandlerLoop(loopProcTmp);

	return status;
}

static Wifi_RespStatus sendInstruction(const char* instruction, const uint32_t timeout)
{
	if(HAL_OK != UART_TransmitLine(&WiFiUart, instruction, timeout))
	{
		return WIFI_RESP_ERROR;
	}
	return waitForResp(timeout);
}

static time_t parseWiFiTime(const char* timeStr)
{
	struct tm timeStruct;

	timeStr+=4;
	if(0 == strncmp(timeStr, "Jan", 3)){ timeStruct.tm_mon = 0; }
	else if(0 == strncmp(timeStr, "Feb", 3)) { timeStruct.tm_mon = 1; }
	else if(0 == strncmp(timeStr, "Mar", 3)) { timeStruct.tm_mon = 2; }
	else if(0 == strncmp(timeStr, "Apr", 3)) { timeStruct.tm_mon = 3; }
	else if(0 == strncmp(timeStr, "May", 3)) { timeStruct.tm_mon = 4; }
	else if(0 == strncmp(timeStr, "Jun", 3)) { timeStruct.tm_mon = 5; }
	else if(0 == strncmp(timeStr, "Jul", 3)) { timeStruct.tm_mon = 6; }
	else if(0 == strncmp(timeStr, "Aug", 3)) { timeStruct.tm_mon = 7; }
	else if(0 == strncmp(timeStr, "Sep", 3)) { timeStruct.tm_mon = 8; }
	else if(0 == strncmp(timeStr, "Oct", 3)) { timeStruct.tm_mon = 9; }
	else if(0 == strncmp(timeStr, "Nov", 3)) { timeStruct.tm_mon = 10; }
	else if(0 == strncmp(timeStr, "Dec", 3)) { timeStruct.tm_mon = 11; }

	timeStr+=4;
	timeStruct.tm_mday = atoi(timeStr);

	timeStr+=3;
	timeStruct.tm_hour = atoi(timeStr);

	timeStr+=3;
	timeStruct.tm_min = atoi(timeStr);

	timeStr+=3;
	timeStruct.tm_sec = atoi(timeStr);

	timeStr+=3;
	timeStruct.tm_year = atoi(timeStr) - 1900;

	return mktime(&timeStruct);
}

void UART_NewLineReceivedCallback(UART_HandleTypeDef *huart)
{
	//If that was status msg, handle it and wait for next one
	if(WIFI_RESP_OK == WiFi_HandleMessage((char*)huart->pRxBuffPtr, huart->RxXferCount))
	{
		UART_ReceiveLine_IT(huart, huart->pRxBuffPtr, huart->RxXferSize);
	}
	else
	{
		g_newMessage = true;
		if(g_handlerLoopRunning)
		{
			UART_ReceiveLine_IT(huart, huart->pRxBuffPtr, huart->RxXferSize);
		}
	}
}

Wifi_RespStatus WiFi_MessageReceivingStart(bool loopMode)
{
	g_newMessage = false;
	if(HAL_OK == UART_ReceiveLine_IT(&WiFiUart, (uint8_t*) espMsgBuff, WIFI_MESSAGE_BUFFER_SIZE))
	{
		g_handlerLoopRunning = loopMode;
		return WIFI_RESP_OK;
	}
	else
	{
		return WIFI_RESP_ERROR;
	}
}

Wifi_RespStatus WiFi_MessageReceivingStop(void)
{
	if(HAL_OK == UART_AbortReceiveLine_IT(&WiFiUart))
	{
		g_handlerLoopRunning = false;
		return WIFI_RESP_OK;
	}
	else
	{
		return WIFI_RESP_ERROR;
	}
}

Wifi_RespStatus WiFi_restart(uint32_t timeout)
{
	Wifi_RespStatus status = WIFI_RESP_ERROR;
//	char respBuff[32];
	char* message = NULL;

	WiFi_MessageReceivingStop();

	HAL_GPIO_WritePin(WiFi_EN_GPIO, WiFi_EN_PIN, GPIO_PIN_RESET);
	system_sleep(500);
	HAL_GPIO_WritePin(WiFi_EN_GPIO, WiFi_EN_PIN, GPIO_PIN_SET);
	system_sleep(1000);

	DMA_memset((void*)&wifiStatus, 0, sizeof(SWiFiStatus));

	uint32_t startTime = HAL_GetTick();

	if(HAL_OK != UART_TransmitLine(&WiFiUart, "AT+RST", 1000)){
		return status;
	};

	WiFi_MessageReceivingStart(true);
	do
	{
		if(messageReceived())
		{
			message = getMessage();
			if(0 == strcmp(message, "ready"))
			{
				status = WIFI_RESP_OK;
				break;
			}
		}
		else
		{
			//HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
		}
	}
	while(startTime + timeout > HAL_GetTick());
	WiFi_MessageReceivingStop();


//
//	volatile int i = 0;
//	char potato1[64];
//	char potato2[64];
//	char potato3[64];
//
//	while(HAL_GetTick()-startTime <= timeout)
//	{
//		char* buff;
//		switch (i%3) {
//			case 0:
//				buff = potato1;
//				break;
//			case 1:
//				buff = potato2;
//				break;
//			case 2:
//				buff = potato3;
//				break;
//		}
//		i++;
//
////		if(HAL_OK == UART_ReceiveLine(&WiFiUart, (uint8_t*)respBuff, sizeof(respBuff), timeout))
//		if(HAL_OK == UART_ReceiveLine(&WiFiUart, (uint8_t*)buff, 64, timeout))
//		{
//			if(strcmp(buff, "ready") == 0){
//				status = WIFI_RESP_OK;
//				break;
//			}
//		}
//	}

	if(status == WIFI_RESP_OK)
	{
		//station mode
		if(WIFI_RESP_OK != (status = sendInstruction("AT+CWMODE=1", 500)))
		{
			return status;
		}

#ifndef DEBUG
		sendInstruction("ATE0", 1000);	//switch echo off
#endif
		//multiconnection mode
		sendInstruction("AT+CIPMUX=1", timeout);

		//passive receiving mode on
		sendInstruction("AT+CIPRECVMODE=1", timeout);
	}

	return status;
}

void WiFi_shutdown(void)
{
	WiFi_MessageReceivingStop();
	HAL_GPIO_WritePin(WiFi_EN_GPIO, WiFi_EN_PIN, GPIO_PIN_RESET);
}


Wifi_RespStatus WiFi_AP_mode(SWiFi_AP_Config* ap_conf)
{
	Wifi_RespStatus status = WIFI_RESP_OK;
	char command[256];
	if(ap_conf == NULL)
	{
		return WIFI_RESP_ERROR;
	}

	//station & AP mode
	if(WIFI_RESP_OK != (status = sendInstruction("AT+CWMODE=3", 500)))
	{
		return status;
	}

	sprintf(command, "AT+CWSAP=\"%s\",\"%s\",%d,%d,%d",
			ap_conf->ssid,
			ap_conf->pass,
			ap_conf->channel,
			ap_conf->enc,
			ap_conf->maxConnection
	);
	if(WIFI_RESP_OK != (status = sendInstruction(command, 2000)))
	{
		return status;
	}

	return status;
}

int8_t WiFi_OpenTCPConnection(char * address, uint16_t port, uint16_t keepAlive)
{
	char command[256];
	int8_t linkID = -1;

	if(WIFI_RESP_OK != WiFi_UpdateStatus(1000))
	{
		return -1;
	}

	//get free link
	for(int i = 0; i < 5; i++)
	{
		if((wifiStatus.linksStatus[i].connected == false) && (wifiStatus.linksStatus[i].dataWaiting <= 0))
		{
			sprintf(command, "AT+CIPSTART=%d,\"TCP\",\"%s\",%d,%d", i, address, port, keepAlive);
			if(WIFI_RESP_OK == sendInstruction(command, 1000))
			{
				linkID = i;
				break;
			}
		}
	}

	return linkID;
}

Wifi_RespStatus WiFi_OpenServerSocket(uint16_t port, uint8_t maxConn, uint16_t serverTimeout, uint32_t timeout)
{
	Wifi_RespStatus status = WIFI_RESP_OK;
	char command[30];

	//max server connection
	sprintf(command, "AT+CIPSERVERMAXCONN=%d", maxConn);
	if(WIFI_RESP_OK !=  (status = sendInstruction(command, timeout)))
	{
		return status;
	}

	//server start
	sprintf(command, "AT+CIPSERVER=1,%d", port);
	if(WIFI_RESP_OK !=  (status = sendInstruction(command, timeout)))
	{
		return status;
	}

	//set connection timeout
	sprintf(command, "AT+CIPSTO=%d", (uint16_t)serverTimeout);
	if(WIFI_RESP_OK !=  (status = sendInstruction(command, timeout)))
	{
		return status;
	}

//	//passive receiving mode on
//	if(WIFI_RESP_OK !=  (status = sendInstruction("AT+CIPRECVMODE=1", timeout)))
//	{
//		return status;
//	}

	return status;
}

Wifi_RespStatus WiFi_CloseServerSocket(uint16_t port)
{
	const uint32_t timeout = 500;
	char command[24];

	//server start
	sprintf(command, "AT+CIPSERVER=0,%d", port);
	return sendInstruction(command, timeout);
}

Wifi_RespStatus WiFi_HandleMessage(char* message, uint32_t size)
{
	if(size < 4) return WIFI_RESP_ERROR;
	if(*message >= '0' && *message <= '4')
	{
		if(*(message+3) == 'O')// || *(message+3) == 'L')	//<con_id>,C[O]NNECT || <con_id>,C[L]OSED
		{
			return WIFI_RESP_OK;
		}
		else if(*(message+3) == 'L')
		{
			uint8_t linkId = (uint8_t)(*message) - (uint8_t)'0';
			if(linkId > 4){ return WIFI_RESP_ERROR; }
			wifiStatus.linksStatus[linkId].connected= false;
			return WIFI_RESP_OK;
		}
		return WIFI_RESP_ERROR;
	}

	//+IPD,<link ID>,<len>
	if(strncmp(message, "+IPD,", 5) == 0)
	{
		return WIFI_RESP_OK;
	}

	if(0 == strncmp(message, "SEND OK", 7))
	{
		sendingComplete = true;
		if(asyncSend)
		{
			asyncSend = false;
			return WIFI_RESP_OK;
		}
	}
//	else if(0 == strncmp(message, "SEND FAIL", 9))
//	{
//		sendingComplete = true;
//		return WIFI_RESP_OK;
//	}
//	else if(0 == strncmp(message, "ERROR", 5))
//	{
//		sendingComplete = true;
//		return WIFI_RESP_OK;
//	}
	return WIFI_RESP_ERROR;

//	if(size < 4) return WIFI_RESP_ERROR;
//
//	// open/close connection event
//	if(*message >= '0' && *message <= '4')
//	{
//		uint8_t linkId = (uint8_t)(*message) - (uint8_t)'0';
//
//		if(*(message+3) == 'O')	//<con_id>,C[O]NNECT
//		{
//			wifiStatus.linksStatus[linkId].connected= true;
//		}
//		else if(*(message+3) == 'L')	//<con_id>,C[L]OSED
//		{
//			wifiStatus.linksStatus[linkId].connected= false;
//		}
//		else
//		{
//			return WIFI_RESP_ERROR;
//		}
//		return WIFI_RESP_OK;
//	}
//
//	//+IPD,<link ID>,<len>
//	if(strncmp(message, "+IPD,", 5) == 0)
//	{
//		uint8_t linkId = (uint8_t)(*(message+5)) - (uint8_t)'0';
//		uint32_t receivedData = atoi(message + 7);
//		if(linkId > 4 && receivedData == 0)
//		{
//			return WIFI_RESP_ERROR;
//		}
//		wifiStatus.linksStatus[linkId].dataWaiting = receivedData;
//		return WIFI_RESP_OK;
//	}
//
//	return WIFI_RESP_ERROR;
}

static inline char readChar(){
	char buffer = '\0';
	HAL_UART_Receive(&WiFiUart, (uint8_t *)&buffer, 1, 500);
	return buffer;
}

int32_t WiFi_ReadData(uint8_t linkID, uint8_t* data, uint16_t dataSize, uint32_t timeout)
{
	const char headerPattern[] = "+CIPRECVDATA:";
	const uint8_t headerPatternSize = 13;

	bool handlerLoopTmp = WiFi_MessageReceivingIsRunning();

	int32_t rcvLength = -1;

	char cmdBuff[32];
	uint8_t charCounter = 0;
	bool headerRecived = false;
	uint32_t startTime;

	sprintf(cmdBuff, "AT+CIPRECVDATA=%d,%d", linkID, dataSize);

	if(HAL_OK != UART_TransmitLine(&WiFiUart, cmdBuff, timeout))
	{
		return -1;
	}


	WiFi_MessageReceivingStop();
	startTime = HAL_GetTick();

	//receiving of header +CIPRECVDATA,
	while(startTime + timeout > HAL_GetTick())
	{
		if(readChar() == headerPattern[charCounter++])
		{

			if(charCounter == headerPatternSize)
			{
				charCounter = 0;
				headerRecived = true;
				break;
			}
		}
		else
		{
			charCounter = 0;
		}

	}

	//get data length
	if(headerRecived)
	{
		while(startTime + timeout > HAL_GetTick())
		{
			cmdBuff[charCounter] = readChar();
			if(cmdBuff[charCounter] == ',')
			{
				rcvLength = atoi(cmdBuff);
				break;
			}
			charCounter++;
			if(charCounter > 6){
				restoreHandlerLoop(handlerLoopTmp);
				return -1;
			}
		}
	}
	else
	{
		restoreHandlerLoop(handlerLoopTmp);
		return -1;
	}

	//receive data
	HAL_UART_Receive_DMA(&WiFiUart, data, rcvLength);
	while(HAL_UART_STATE_BUSY_RX == HAL_UART_GetState(&WiFiUart))
	{
		if(startTime + timeout < HAL_GetTick())
		{
			HAL_UART_AbortReceive(&WiFiUart);
			restoreHandlerLoop(handlerLoopTmp);
			return -1;
		}
		HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
	}

	wifiStatus.linksStatus[linkID].dataWaiting -= rcvLength;

	restoreHandlerLoop(handlerLoopTmp);
	return rcvLength;
}

Wifi_RespStatus WiFi_SendData(uint8_t linkID, uint8_t* data, uint16_t dataSize)
{
	return WiFi_SendDataEx(linkID, data, dataSize, true);
}

Wifi_RespStatus WiFi_SendDataEx(uint8_t linkID, uint8_t* data, uint16_t dataSize, bool wait)
{
	const uint32_t timeout = 5000;
	char commandBuff[24];

	Wifi_RespStatus status = WIFI_RESP_TIMEOUT;
	char* message;

	uint32_t startTime = HAL_GetTick();
	bool handlerLoopTmp = WiFi_MessageReceivingIsRunning();


	sprintf(commandBuff, "AT+CIPSEND=%d,%d", linkID, dataSize);
	if(HAL_OK != UART_TransmitLine(&WiFiUart, commandBuff, 100))
	{
		return WIFI_RESP_ERROR;
	}

	WiFi_MessageReceivingStart(false);
	status = WIFI_RESP_TIMEOUT;
	do
	{
		if(messageReceived())
		{
			message = getMessage();
			if(0 == strcmp(message, "OK"))
			{
				break;
			}
			else if(0 == strncmp(message, "link", 4))
			{
				return WIFI_RESP_ERROR;
			}
		}
	} while(startTime + timeout > HAL_GetTick());

	WiFi_MessageReceivingStop();

	char startChar;
	while(1)
	{

		if(startTime + timeout < HAL_GetTick())
		{
			restoreHandlerLoop(handlerLoopTmp);
			return WIFI_RESP_TIMEOUT;
		}
		startChar = readChar();
		if(startChar == '>')
		{
			break;
		}
	}


	if(HAL_OK != HAL_UART_Transmit_DMA(&WiFiUart, data, dataSize))
	{
		restoreHandlerLoop(handlerLoopTmp);
		return status;
	}
	else
	{
		sendingComplete = false;
		if(!wait)
		{
			asyncSend = true;
			WiFi_MessageReceivingStart(true);
			return WIFI_RESP_OK;
		}
	}

	WiFi_MessageReceivingStart(false);
	status = WIFI_RESP_TIMEOUT;
	do
	{
		if(messageReceived())
		{
			if(0 == strcmp(getMessage(), "SEND OK"))
			{
				status = WIFI_RESP_OK;
				break;
			}
			else if(0 == strcmp(getMessage(), "SEND FAIL"))
			{
				status = WIFI_RESP_ERROR;
				break;
			}
			else if(0 == strcmp(getMessage(), "ERROR"))
			{
				status = WIFI_RESP_ERROR;
				break;
			}
			else
			{
				WiFi_MessageReceivingStart(false);
			}
		}
		else
		{
			HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
		}
	}
	while(startTime + timeout > HAL_GetTick());

	restoreHandlerLoop(handlerLoopTmp);

	return status;
}

Wifi_RespStatus WiFi_CloseConnection(uint8_t linkID)
{
	const uint32_t timeout = 500;
	char command[24];

	//server start
	sprintf(command, "AT+CIPCLOSE=%d", linkID);
	return sendInstruction(command, timeout);
}

bool WiFi_MessageReceivingIsRunning(void)
{
	return g_handlerLoopRunning;
}

Wifi_RespStatus WiFi_UpdateStatus(const uint32_t timeout)
{
	Wifi_RespStatus status = WIFI_RESP_TIMEOUT;
	uint32_t startTime = HAL_GetTick();
	char* message = NULL;

	for(uint8_t i = 0; i < 5; i++)
	{
		wifiStatus.linksStatus[i].connected = false;
	}

	//check WiFi AP connection
	if(HAL_OK != UART_TransmitLine(&WiFiUart, "AT+CWSTATE?", 100))
	{
		return WIFI_RESP_ERROR;
	}
	WiFi_MessageReceivingStart(true);

	do
	{
		if(messageReceived())
		{
			message = getMessage();
			if(0 == strcmp(message, "OK"))
			{
				status = WIFI_RESP_OK;
				break;
			}
			else if(0 == strncmp(message, "+CWSTATE:", 9))
			{
				uint8_t connToAPStatus = atoi((message+9));
				wifiStatus.connectedToAP = connToAPStatus != 2 ? false : true;
			}
		}
		else
		{
			HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
		}
	}
	while(startTime + timeout > HAL_GetTick());

	//check TCP connection
	if(HAL_OK != UART_TransmitLine(&WiFiUart, "AT+CIPSTATE?", 100))
	{
		return WIFI_RESP_ERROR;
	}
	WiFi_MessageReceivingStart(true);

	do
	{
		if(messageReceived())
		{
			message = getMessage();
			if(0 == strcmp(message, "OK"))
			{
				status = WIFI_RESP_OK;
				break;
			}
			else if(0 == strncmp(message, "+CIPSTATE:", 10))
			{
				uint8_t linkID = atoi((message+10));
				wifiStatus.linksStatus[linkID].connected = true;
			}
		}
		else
		{
			HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
		}
	}
	while(startTime + timeout > HAL_GetTick());

	if(WIFI_RESP_OK != status)
	{
		return status;
	}

	//size of received data
	if(HAL_OK != UART_TransmitLine(&WiFiUart, "AT+CIPRECVLEN?", 100))
	{
		return WIFI_RESP_ERROR;
	}

	WiFi_MessageReceivingStart(true);

	status = WIFI_RESP_TIMEOUT;
	do
	{
		if(messageReceived())
		{
			message = getMessage();
			if(0 == strncmp(message, "+CIPRECVLEN:", 12))
			{
				message += 12;
				for(uint8_t i = 0; i < 5; i++)
				{
					wifiStatus.linksStatus[i].dataWaiting = atoi(message);
					while(1)
					{
						if(*message == ',')
						{
							message++;
							break;
						}
						else if(*message == '\0')
						{
							break;
						}
						else
						{
							message++;
						}
					}
				}
				status = WIFI_RESP_OK;
				break;
			}
		}
		else
		{
			HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
		}
	}
	while(startTime + timeout > HAL_GetTick());

	return status;
}

Wifi_RespStatus WiFi_GetVersionString(char* versionStringPtr, uint16_t size, uint32_t timeout)
{
	uint32_t startTime = HAL_GetTick();
	Wifi_RespStatus status = WIFI_RESP_TIMEOUT;
	char* message = NULL;

	if(HAL_OK != UART_TransmitLine(&WiFiUart, "AT+GMR", 100))
	{
		return WIFI_RESP_ERROR;
	}

	WiFi_MessageReceivingStart(true);
	do
	{
		if(messageReceived())
		{
			message = getMessage();
			if(0 == strncmp(message, "Bin version:", 12))
			{
				message += 12;
				uint16_t verStrSize = 0;
				while(*(message+verStrSize) != '\n' && verStrSize < size)
				{
					verStrSize++;
				}

				strncpy(versionStringPtr, message, size);
				*(versionStringPtr+size-1) = '\0';
				status = WIFI_RESP_OK;
				break;
			}
			if(0 == strcmp(message, "OK"))
			{
				status = WIFI_RESP_ERROR;
				break;
			}
		}
		else
		{
			HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
		}
	}
	while(startTime + timeout > HAL_GetTick());

	return status;

}

Wifi_RespStatus WiFi_DisconnectStation(uint32_t timeout)
{
	return sendInstruction("AT+CWQAP", timeout);
}

Wifi_RespStatus WiFi_ConnectToStation(char* ssid, char* pass, uint8_t* connectStatus, uint32_t timeout)
{
	const uint32_t startTime = HAL_GetTick();
	char command[250];
	char* message = NULL;

	*connectStatus = 4;


	sprintf(command, "AT+CWJAP=\"%s\",\"%s\"", ssid, pass);
	if(HAL_OK != UART_TransmitLine(&WiFiUart, command, 100))
	{
		return WIFI_RESP_ERROR;
	}

	WiFi_MessageReceivingStart(true);
	do
	{
		if(messageReceived())
		{
			message = getMessage();
			if(0 == strncmp(message, "+CWJAP:", 7))
			{
				*connectStatus = atoi((message + 7));
			}
			else if(0 == strncmp(message, "FAIL", 4))
			{
				return WIFI_RESP_OK;
			}
			else if(0 == strncmp(message, "OK", 2))
			{
				*connectStatus = 0;
				return WIFI_RESP_OK;
			}
		}
		else
		{
			HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
		}
	}
	while(startTime + timeout > HAL_GetTick());

	return WIFI_RESP_TIMEOUT;
}

Wifi_RespStatus WiFi_GetAPList(SWiFiStation* stationlist, uint8_t* listSize, uint32_t timeout)
{
	uint32_t startTime = HAL_GetTick();
	Wifi_RespStatus status = WIFI_RESP_TIMEOUT;
	char* message = NULL;
	const uint8_t maxListSize = *listSize;
	uint8_t listCounter = 0;

	status = sendInstruction("AT+CWLAPOPT=1,7", timeout);	//show only rssi, ssid, enc in rssi order
	if(status != WIFI_RESP_OK)
	{
		return status;
	}

	if(HAL_OK != UART_TransmitLine(&WiFiUart, "AT+CWLAP", 100))
	{
		return WIFI_RESP_ERROR;
	}

	WiFi_MessageReceivingStart(true);
	do
	{
		if(messageReceived())
		{
			message = getMessage();
			if(0 == strncmp(message, "+CWLAP:(", 8))
			{
				message += 8;
				SWiFiStation* stationInfo = (stationlist + listCounter);

				stationInfo->enc = (WiFi_Encryption) atoi(message);
				if(stationInfo->enc >= WIFI_ENC_NUM)
				{
					continue;
				}
				message += 3;
				char* endOfssid = message;
				for(uint8_t i = strlen(message); i != 0; i--)
				{
					if(*(message + i) == '"')
					{
						endOfssid = (message + i);
						break;
					}
				}
				uint8_t ssidSize = endOfssid-message;

				DMA_memcpy(stationInfo->ssid, message, ssidSize);
				stationInfo->ssid[ssidSize] = '\0';

				message += (ssidSize + 2);
				stationInfo->rssi = atoi(message);

				listCounter++;
				if(listCounter >= maxListSize)
				{
					break;
				}
			}
			if(0 == strcmp(message, "OK"))
			{
				break;
			}
		}
		else
		{
			HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
		}
	}
	while(startTime + timeout > HAL_GetTick());

	*listSize = listCounter;
	return status;
}

Wifi_RespStatus WiFi_EnableDHCP(uint8_t dhcpEnable, uint32_t timeout)
{
	char command[20];
	sprintf(command, "AT+CWDHCP=1,%d", dhcpEnable? 1 : 0);
	return sendInstruction(command, timeout);
}

Wifi_RespStatus WiFi_SetIP(char* ip, char* mask, char* gateway, uint32_t timeout)
{
	char command[70];
	sprintf(command, "AT+CIPSTA=\"%s\",\"%s\",\"%s\"", ip, gateway, mask);
	return sendInstruction(command, timeout);
}

Wifi_RespStatus WiFi_setStationName(char* name, uint32_t timeout)
{
	char command[50];
	sprintf(command, "AT+CWHOSTNAME=\"%s\"", name);
	return sendInstruction(command, timeout);
}

Wifi_RespStatus WiFi_getIPAddress(char* ip, uint32_t timeout)
{
	uint32_t startTime = HAL_GetTick();
	bool flag = false;
	char* message = NULL;

	if(HAL_OK != UART_TransmitLine(&WiFiUart, "AT+CIPSTA?", 100))
	{
		return WIFI_RESP_ERROR;
	}

	WiFi_MessageReceivingStart(true);
	do
	{
		if(messageReceived())
		{
			message = getMessage();
			if(0 == strncmp(message, "+CIPSTA:ip:\"", 12))
			{
				message = (message + 12);
				char* endPtr = strchr(message, '"');
				if(NULL == endPtr) return WIFI_RESP_ERROR;
				DMA_memcpy(ip, message, endPtr - message);
				ip[endPtr - message] = '\0';
				flag = true;
			}
			else if(0 == strncmp(message, "FAIL", 4))
			{
				return WIFI_RESP_ERROR;
			}
			else if(0 == strncmp(message, "OK", 2))
			{
				if(flag)
				{
					return WIFI_RESP_OK;
				}
				else
				{
					ip[0] = '\0';
					return WIFI_RESP_ERROR;
				}
			}
		}
		else
		{
			HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
		}
	}
	while(startTime + timeout > HAL_GetTick());

	return WIFI_RESP_TIMEOUT;
}

Wifi_RespStatus WiFi_setStationAutoConnection(uint8_t enable, uint32_t timeout)
{
	char command[16];
	sprintf(command, "AT+CWAUTOCONN=%d", enable? 1 : 0);
	return sendInstruction(command, timeout);
}

Wifi_RespStatus WiFi_setSNTPconfig(uint8_t enable, int8_t timezone, uint32_t timeout)
{
	char command[255];
	if(enable && (timezone < -11 || timezone > 13))
	{
		return WIFI_RESP_ERROR;
	}
	if(enable)
	{
		sprintf(command, "AT+CIPSNTPCFG=1,%d,\"%s\",\"%s\",\"%s\"", timezone, SNTP_ADDRESS_1, SNTP_ADDRESS_2, SNTP_ADDRESS_3);
	}
	else
	{
		sprintf(command, "AT+CIPSNTPCFG=0");
	}
	return sendInstruction(command, timeout);
}

Wifi_RespStatus WiFi_getSNTPtime(time_t* timeSecPtr, uint32_t timeout)
{
	uint32_t startTime = HAL_GetTick();
	char* message = NULL;

	if(HAL_OK != UART_TransmitLine(&WiFiUart, "AT+CIPSNTPTIME?", 100))
	{
		return WIFI_RESP_ERROR;
	}

	WiFi_MessageReceivingStart(true);
	do
	{
		if(messageReceived())
		{
			message = getMessage();
			if(0 == strncmp(message, "+CIPSNTPTIME:", 13))
			{
				message = (message + 13);
				*timeSecPtr = parseWiFiTime(message);
				if(*timeSecPtr == -1)
				{
					return WIFI_RESP_ERROR;
				}
				return WIFI_RESP_OK;

			}
			else if(0 == strncmp(message, "OK", 2))
			{
				return WIFI_RESP_ERROR;
			}
		}
		else
		{
			HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
		}
	}
	while(startTime + timeout > HAL_GetTick());

	return WIFI_RESP_TIMEOUT;
}

bool WiFi_SendingComplete(void)
{
	while(!sendingComplete)
	{
		HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
	}
	return sendingComplete;
}

//void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
//{
//	//sendingComplete = true;
//}
