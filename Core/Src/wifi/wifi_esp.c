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

#include "logger.h"


#define WIFI_MESSAGE_BUFFER_SIZE 64

volatile SWiFiStatus wifiStatus;

UART_RB_HandleTypeDef uart_rb;

static char espMsgBuff[WIFI_MESSAGE_BUFFER_SIZE];

static volatile bool sendingComplete = true;
static volatile bool asyncSend = false;


static char* getMessage(void);
static Wifi_RespStatus defaultMsgHandler(char* message, uint32_t size);

static Wifi_RespStatus messageReceivingStart(void);
static Wifi_RespStatus messageReceivingStop(void);

static Wifi_RespStatus flushMsgBuffer(void);

static Wifi_RespStatus waitForResp(const uint32_t timeout)
{
    Wifi_RespStatus status = WIFI_RESP_TIMEOUT;
    char *message = NULL;

    uint32_t startTime = HAL_GetTick();

    do
    {
        message = getMessage();
        if(NULL != message)
        {
            if(0 == strcmp(message, "OK"))
            {
                status = WIFI_RESP_OK;
                break;
            }
            else if(0 == strcmp(message, "ERROR"))
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

static Wifi_RespStatus sendInstruction(const char* instruction, const uint32_t timeout)
{
    WiFi_handleMessages();
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

static char* getMessage(void)
{
    UART_RB_StatusTypeDef status = UART_RB_OK;
    uint16_t size = 0;

    do
    {
        status = uart_rb_getNextLine(&uart_rb, espMsgBuff, sizeof(espMsgBuff), &size);

        if(status == UART_RB_EMPTY)
        {
            return NULL;
        }

        if(status != UART_RB_OK)
        {
            Logger(LOG_ERR, "RB error! %u %s", status, __FUNCTION__);
            return NULL;
        }
    }
    while(WIFI_RESP_OK == defaultMsgHandler(espMsgBuff, size));

    return espMsgBuff;
}

static Wifi_RespStatus flushMsgBuffer(void)
{
    if(UART_RB_OK == uart_rb_flush(&uart_rb))
    {
        return WIFI_RESP_OK;
    }
    else
    {
        return WIFI_RESP_ERROR;
    }
}

static Wifi_RespStatus messageReceivingStart(void)
{
    if(UART_RB_OK == uart_rb_start(&uart_rb, &WiFiUart))
    {
        return WIFI_RESP_OK;
    }
    else
    {
        return WIFI_RESP_ERROR;
    }
}

static Wifi_RespStatus messageReceivingStop(void)
{
    if(UART_RB_OK == uart_rb_stop(&uart_rb))
    {
        return WIFI_RESP_OK;
    }
    else
    {
        return WIFI_RESP_ERROR;
    }
}

static Wifi_RespStatus defaultMsgHandler(char* message, uint32_t size)
{
    if(size < 4) return WIFI_RESP_ERROR;
    if(*message >= '0' && *message <= '4')
    {
        if(*(message+3) == 'O')// || *(message+3) == 'L')   //<con_id>,C[O]NNECT || <con_id>,C[L]OSED
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

    if(asyncSend)
    {
        if(0 == strncmp(message, "SEND OK", 7)   ||
           0 == strncmp(message, "SEND FAIL", 9) ||
           0 == strncmp(message, "ERROR", 5))
        {
            sendingComplete = true;
            asyncSend = false;
            return WIFI_RESP_OK;
        }
    }

    return WIFI_RESP_ERROR;
}

static inline char readChar(){
    char buffer = '\0';
    HAL_UART_Receive(&WiFiUart, (uint8_t *)&buffer, 1, 500);
    return buffer;
}

Wifi_RespStatus WiFi_handleMessages(void)
{
    UART_RB_StatusTypeDef status = UART_RB_OK;
    uint16_t size = 0;

    while(UART_RB_OK == (status = uart_rb_getNextLine(&uart_rb, espMsgBuff, sizeof(espMsgBuff), &size)))
    {
        defaultMsgHandler(espMsgBuff, size);
    }

    if(status == UART_RB_EMPTY)
    {
        return WIFI_RESP_OK;
    }

    Logger(LOG_ERR, "RB error! %u %s", status, __FUNCTION__);
    return WIFI_RESP_ERROR;
}

Wifi_RespStatus WiFi_restart(uint32_t timeout)
{
    Wifi_RespStatus status = WIFI_RESP_ERROR;
    //	char respBuff[32];
    char* message = NULL;

    messageReceivingStop();

    HAL_GPIO_WritePin(WiFi_EN_GPIO, WiFi_EN_PIN, GPIO_PIN_RESET);
    system_sleep(500);
    HAL_GPIO_WritePin(WiFi_EN_GPIO, WiFi_EN_PIN, GPIO_PIN_SET);
    system_sleep(1000);

    DMA_memset((void*)&wifiStatus, 0, sizeof(SWiFiStatus));

    uint32_t startTime = HAL_GetTick();

    if(HAL_OK != UART_TransmitLine(&WiFiUart, "AT+RST", 1000)){
        return status;
    };

    messageReceivingStart();
    do
    {
        message = getMessage();
        if(NULL != message)
        {
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
    messageReceivingStop();
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

int32_t WiFi_ReadData(uint8_t linkID, uint8_t* data, uint16_t dataSize, uint32_t timeout)
{
    const char headerPattern[] = "+CIPRECVDATA:";
    const uint8_t headerPatternSize = 13;

    int32_t rcvLength = -1;

    char cmdBuff[32];
    uint8_t charCounter = 0;
    bool headerRecived = false;
    uint32_t startTime;

    WiFi_handleMessages();

    messageReceivingStop();

    sprintf(cmdBuff, "AT+CIPRECVDATA=%d,%d", linkID, dataSize);

    if(HAL_OK != UART_TransmitLine(&WiFiUart, cmdBuff, timeout))
    {
        return -1;
    }

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
                messageReceivingStart();
                return -1;
            }
        }
    }
    else
    {
        messageReceivingStart();
        return -1;
    }

    //receive data
    HAL_UART_Receive_DMA(&WiFiUart, data, rcvLength);

    while(HAL_UART_STATE_BUSY_RX == HAL_UART_GetState(&WiFiUart))
    {
        if(startTime + timeout < HAL_GetTick())
        {
            HAL_UART_AbortReceive(&WiFiUart);
            messageReceivingStart();
            return -1;
        }
        HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
    }

    wifiStatus.linksStatus[linkID].dataWaiting -= rcvLength;
    messageReceivingStart();

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

    WiFi_handleMessages();

    sprintf(commandBuff, "AT+CIPSEND=%d,%d", linkID, dataSize);
    if(HAL_OK != UART_TransmitLine(&WiFiUart, commandBuff, 100))
    {
        return WIFI_RESP_ERROR;
    }

    status = WIFI_RESP_TIMEOUT;
    do
    {
        message = getMessage();
        if(NULL != message)
        {
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

    char startChar;
    UART_RB_StatusTypeDef rb_status;
    while(1)
    {

        if(startTime + timeout < HAL_GetTick())
        {
            flushMsgBuffer();
            return WIFI_RESP_TIMEOUT;
        }

        rb_status = uart_rb_getNextChar(&uart_rb, &startChar);

        if(UART_RB_EMPTY == rb_status)
        {
            HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
            continue;
        }
        else if(UART_RB_OK != rb_status)
        {
            Logger(LOG_ERR, "RB error! %u %s", status, __FUNCTION__);
            return WIFI_RESP_ERROR;
        }

        if(startChar == '>')
        {
            break;
        }
    }

    flushMsgBuffer();

    if(HAL_OK != HAL_UART_Transmit_DMA(&WiFiUart, data, dataSize))
    {
        return status;
    }
    else
    {
        sendingComplete = false;
        if(!wait)
        {
            asyncSend = true;
            return WIFI_RESP_OK;
        }
    }

    status = WIFI_RESP_TIMEOUT;
    do
    {
        message = getMessage();
        if(NULL != message)
        {
            if(0 == strcmp(message, "SEND OK"))
            {
                status = WIFI_RESP_OK;
                break;
            }
            else if(0 == strcmp(message, "SEND FAIL"))
            {
                status = WIFI_RESP_ERROR;
                break;
            }
            else if(0 == strcmp(message, "ERROR"))
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


Wifi_RespStatus WiFi_CloseConnection(uint8_t linkID)
{
    const uint32_t timeout = 500;
    char command[24];

    //server start
    sprintf(command, "AT+CIPCLOSE=%d", linkID);
    return sendInstruction(command, timeout);
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

    WiFi_handleMessages();

    //check WiFi AP connection
    if(HAL_OK != UART_TransmitLine(&WiFiUart, "AT+CWSTATE?", 100))
    {
        return WIFI_RESP_ERROR;
    }

    do
    {
        message = getMessage();
        if(NULL != message)
        {
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

    do
    {
        message = getMessage();
        if(NULL != message)
        {
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

    status = WIFI_RESP_TIMEOUT;
    do
    {
        message = getMessage();
        if(NULL != message)
        {
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

    WiFi_handleMessages();

    if(HAL_OK != UART_TransmitLine(&WiFiUart, "AT+GMR", 100))
    {
        return WIFI_RESP_ERROR;
    }

    do
    {
        message = getMessage();
        if(NULL != message)
        {
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

    WiFi_handleMessages();

    sprintf(command, "AT+CWJAP=\"%s\",\"%s\"", ssid, pass);
    if(HAL_OK != UART_TransmitLine(&WiFiUart, command, 100))
    {
        return WIFI_RESP_ERROR;
    }

    do
    {
        message = getMessage();
        if(NULL != message)
        {
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

    do
    {
        message = getMessage();
        if(NULL != message)
        {
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

    WiFi_handleMessages();

    if(HAL_OK != UART_TransmitLine(&WiFiUart, "AT+CIPSTA?", 100))
    {
        return WIFI_RESP_ERROR;
    }

    do
    {
        message = getMessage();
        if(NULL != message)
        {
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

    WiFi_handleMessages();

    if(HAL_OK != UART_TransmitLine(&WiFiUart, "AT+CIPSNTPTIME?", 100))
    {
        return WIFI_RESP_ERROR;
    }

    do
    {
        message = getMessage();
        if(NULL != message)
        {
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
    WiFi_handleMessages();
    return sendingComplete;
}
