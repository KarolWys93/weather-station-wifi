/*
 * httpServer_api.c
 *
 *  Created on: 14.02.2021
 *      Author: Karol
 */

#include "httpServer_api.h"
#include "http_requests.h"
#include "jsmn.h"

#include "../../wifi/wifi_esp.h"
#include "rtc.h"

#include "system.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static uint16_t escAndCpyStr(char* to, char* from)
{
	uint16_t charCounter = 0;
	while(*from != '\0')
	{
		switch (*from)
    	{
			case '\"':
			case '/':
    		case '\\':
    			*to = '\\';
    			to++;
    			charCounter++;
				break;
			default:
				break;
		}
		*to = *from;
		to++;
		from++;
		charCounter++;
	}
	return charCounter;
}

static uint16_t escStrnLen(char* str, uint16_t strMaxSize)
{
	uint16_t charCounter = 0;

	for(uint16_t i = 0; i < strMaxSize; i++)
	{
		char c = str[i];
		if(c == '\0')
		{
			break;
		}
		else if (c == '\\' && i + 1 < strMaxSize)
	    {
	    	charCounter++;
	    	i++;
	    	c = str[i];
	    	switch (c)
	    	{
				case '\"':
				case '/':
	    		case '\\':
	    		case 'b':
	    		case 'f':
	    		case 'r':
	    		case 'n':
	    		case 't':
					break;
				default:
					charCounter++;
					break;
			}
	    }
		else
		{
			charCounter++;
		}
	}
	return charCounter;
}

static HTTP_STATUS sendResponse(uint16_t code, char* data, uint16_t dataSize)
{
	uint32_t headerSize = 0;
	char header[256];

	if(0 > (headerSize = HTTP_createResponseHeader(header, sizeof(header), code, dataSize))) return HTTP_SERVER_ERROR;
	if(0 > (headerSize = HTTP_addHeaderField(header, sizeof(header), "Cache-Control", "no-store"))) return HTTP_SERVER_ERROR;
	if(dataSize > 0)
	{
		if(0 > (headerSize = HTTP_addHeaderField(header, sizeof(header), "Content-Typ", "application/json"))) return HTTP_SERVER_ERROR;
	}

	if(0 > httpServer_sendData(header, headerSize))
	{
		return HTTP_SERVER_OK;
	}
	if(dataSize > 0)
	{
		if(data == NULL) {return HTTP_SERVER_CRITICAL_ERROR;}
		if(0 > httpServer_sendData(data, dataSize))
		{
			return HTTP_SERVER_OK;
		}
	}
	return HTTP_SERVER_OK;
}


// --- SERVER ENDPOINTS --- //


/*
 * {
 *    "wifi_ssid":"Test_WiFi",
 *    "wifi_stat":1,
 *    "bat":21,
 *    "charging":1,
 *    "ver":"v1.2.3",
 *    "ver_wifi":"1.1.1"
 * }
 */
HTTP_STATUS serverAPI_status(char* request, uint32_t reqSize)
{
	FIL file;
	jsmn_parser jsonParser;
	jsmntok_t jsonTokens[11];
	int16_t numOfTokens = 0;
	char configJsonStr[256];
	uint16_t configJsonSize = 0;

	char response[256];

	sprintf(response, "{\"wifi_stat\":%d,\"bat\":%d,\"charging\":%d,\"ver\":\"%s\",\"ver_wifi\":\"",
			wifiStatus.connectedToAP? 1 : 0,
			system_batteryLevel(),
			system_isCharging(),
			VERSION_STR);

	if(WIFI_RESP_OK != WiFi_GetVersionString(response + strlen(response), 20, 1000)) return HTTP_SERVER_ERROR;

	strcat(response, "\",\"ip_addr\":\"");
	if(WIFI_RESP_OK != WiFi_getIPAddress(response + strlen(response), 1000)) return HTTP_SERVER_ERROR;

	if(FR_OK == f_open(&file, FILE_PATH_WIFI_CONFIG, FA_READ))
	{
		f_read(&file, configJsonStr, sizeof(configJsonStr), (UINT*)&configJsonSize);
		f_close(&file);
	}

	jsmn_init(&jsonParser);
	numOfTokens = jsmn_parse(&jsonParser, configJsonStr, configJsonSize, jsonTokens, sizeof(jsonTokens) / sizeof(jsonTokens[0]));
	if(!(0 > numOfTokens || jsonTokens[0].type != JSMN_OBJECT))	//if no tokens or first token isn't object
	{
		jsmntok_t* ssidTokenPtr = jsmn_get_token("ssid", configJsonStr, jsonTokens, numOfTokens);
		if(NULL != ssidTokenPtr)
		{
			sprintf(response + strlen(response), "\",\"wifi_ssid\":\"%.*s",
									ssidTokenPtr->end - ssidTokenPtr->start,
									configJsonStr + ssidTokenPtr->start);
		}
	}

	strcat(response, "\"}");

	return sendResponse(200, response, strlen(response));
}

/*
 * {
 * 	"ssid":"Test AP",
 * 	"enc":4,
 * 	"channel":8
 * }
 */

HTTP_STATUS serverAPI_getAPconfig(char* request, uint32_t reqSize)
{
	FIL file;
	jsmn_parser jsonParser;
	jsmntok_t jsonTokens[10];
	int16_t numOfTokens = 0;

	char configJsonStr[256];
	uint16_t configJsonSize = 0;

	char responeJsonStr[128] = "{ ";
	char* responeJsonStrPtr = responeJsonStr + strlen(responeJsonStr);


	jsmn_init(&jsonParser);

	if(FR_OK == f_open(&file, FILE_PATH_AP_CONFIG, FA_READ))
	{
		if(FR_OK != f_read(&file, configJsonStr, sizeof(configJsonStr), (UINT*)&configJsonSize))
		{
			f_close(&file);
			return HTTP_SERVER_ERROR;
		}
		f_close(&file);
	}
	else
	{
		return HTTP_SERVER_ERROR;
	}

	numOfTokens = jsmn_parse(&jsonParser, configJsonStr, configJsonSize, jsonTokens, sizeof(jsonTokens) / sizeof(jsonTokens[0]));
	if(0 > numOfTokens || jsonTokens[0].type != JSMN_OBJECT)	//if no tokens or first token isn't object
	{
		return HTTP_SERVER_ERROR;
	}

	for(uint16_t i = 1; i < numOfTokens; i++)
	{
		if(0 == jsmn_eq(configJsonStr, &jsonTokens[i], "ssid"))
		{
			responeJsonStrPtr += sprintf(responeJsonStrPtr, "\"ap_ssid\":\"%.*s\",",
					jsonTokens[i + 1].end - jsonTokens[i + 1].start,
					configJsonStr + jsonTokens[i + 1].start);
			i++;
		}
		else if(0 == jsmn_eq(configJsonStr, &jsonTokens[i], "enc"))
		{
			uint8_t encVal = atoi(configJsonStr + jsonTokens[i + 1].start);
			responeJsonStrPtr += sprintf(responeJsonStrPtr, "\"ap_enc\":%d,", encVal);
			i++;
		}
		else if(0 == jsmn_eq(configJsonStr, &jsonTokens[i], "channel"))
		{
			uint8_t channelVal = atoi(configJsonStr + jsonTokens[i + 1].start);
			responeJsonStrPtr += sprintf(responeJsonStrPtr, "\"ap_chl\":%d,", channelVal);
			i++;
		}
		else
		{
			i++;
		}
	}

	*(responeJsonStrPtr-1) = '}';

	return sendResponse(200, responeJsonStr, strlen(responeJsonStr));
}

HTTP_STATUS serverAPI_setAPconfig(char* request, uint32_t reqSize)
{
	uint8_t tokensToHandle = 4;

	char* requestBodyPtr = NULL;
	uint16_t requestBodySize;

	jsmn_parser jsonParser;
	jsmntok_t jsonTokens[10];
	int16_t numOfTokens = 0;

	FIL file;
	char configJsonStr[256] = "{";
	char* configJsonStrPtr = configJsonStr + strlen(configJsonStr);

	WiFi_Encryption encryptionMethod = WIFI_ENC_OPEN;

	requestBodySize = HTTP_getContentSize(request, reqSize);
	requestBodyPtr = HTTP_getContent(request, reqSize);

	if(requestBodyPtr == NULL || ((requestBodyPtr - request + requestBodySize) != reqSize))
	{
		return HTTP_SERVER_BAD_REQUEST;
	}

	jsmn_init(&jsonParser);

	numOfTokens = jsmn_parse(&jsonParser, requestBodyPtr, requestBodySize, jsonTokens, sizeof(jsonTokens) / sizeof(jsonTokens[0]));
	if(0 > numOfTokens || jsonTokens[0].type != JSMN_OBJECT)	//if no tokens or first token isn't object
	{
		return HTTP_SERVER_BAD_REQUEST;
	}

	uint16_t tokenSize;
	char* tokenPtr;

	uint16_t passTokenSize;
	char* passTokenPtr;

	for(uint16_t i = 1; i < numOfTokens; i++)
	{
		if(0 == jsmn_eq(requestBodyPtr, &jsonTokens[i], "ap_ssid"))
		{
			tokenSize = jsonTokens[i + 1].end - jsonTokens[i + 1].start;
			tokenPtr = requestBodyPtr + jsonTokens[i + 1].start;
			uint16_t size = escStrnLen(tokenPtr, tokenSize);

			if(size > 32)
			{
				return HTTP_SERVER_BAD_REQUEST;
			}
			configJsonStrPtr += sprintf(configJsonStrPtr, "\"ssid\":\"%.*s\",", tokenSize, tokenPtr);
			i++;
			tokensToHandle--;
		}
		else if(0 == jsmn_eq(requestBodyPtr, &jsonTokens[i], "ap_pass"))
		{
			passTokenSize = jsonTokens[i + 1].end - jsonTokens[i + 1].start;
			passTokenPtr = requestBodyPtr + jsonTokens[i + 1].start;
			i++;
		}
		else if(0 == jsmn_eq(requestBodyPtr, &jsonTokens[i], "ap_enc"))
		{
			tokenPtr = requestBodyPtr + jsonTokens[i + 1].start;
			encryptionMethod = atoi(tokenPtr);
			switch (encryptionMethod)
			{
				case WIFI_ENC_OPEN:
				case WIFI_ENC_WPA_PSK:
				case WIFI_ENC_WPA2_PSK:
				case WIFI_ENC_WPA_WPA2_PSK:
					configJsonStrPtr += sprintf(configJsonStrPtr, "\"enc\":%d,", encryptionMethod);
					i++;
					tokensToHandle--;
					break;
				default:
					return HTTP_SERVER_BAD_REQUEST;
			}
		}
		else if(0 == jsmn_eq(requestBodyPtr, &jsonTokens[i], "ap_chl"))
		{
			tokenPtr = requestBodyPtr + jsonTokens[i + 1].start;
			uint8_t channelVal = atoi(requestBodyPtr + jsonTokens[i + 1].start);
			if(channelVal < 1 || channelVal > 14)
			{
				return HTTP_SERVER_BAD_REQUEST;
			}
			configJsonStrPtr += sprintf(configJsonStrPtr, "\"channel\":%d,", channelVal);
			i++;
			tokensToHandle--;
		}
		else
		{
			return HTTP_SERVER_BAD_REQUEST;
		}
	}

	if(WIFI_ENC_OPEN == encryptionMethod)
	{
		configJsonStrPtr += sprintf(configJsonStrPtr, "\"pass\":\"\",");
		tokensToHandle--;
	}
	else
	{
		uint16_t passwordSize =  escStrnLen(passTokenPtr, passTokenSize);
		if(passwordSize > 63 || passwordSize < 8)
		{
			return HTTP_SERVER_BAD_REQUEST;
		}
		configJsonStrPtr += sprintf(configJsonStrPtr, "\"pass\":\"%.*s\",", passTokenSize, passTokenPtr);
		tokensToHandle--;
	}

	*(configJsonStrPtr-1) = '}';

	if(tokensToHandle != 0)
	{
		return HTTP_SERVER_BAD_REQUEST;
	}

	if(FR_OK == f_open(&file, FILE_PATH_AP_CONFIG, FA_OPEN_ALWAYS | FA_WRITE))
	{
		UINT writtenBytes = 0;
		if(FR_OK != f_write(&file, configJsonStr, sizeof(configJsonStr), &writtenBytes) || sizeof(configJsonStr) != writtenBytes )
		{
			f_close(&file);
			return HTTP_SERVER_ERROR;
		}
		f_close(&file);
	}
	else
	{
		return HTTP_SERVER_ERROR;
	}

	return sendResponse(204, NULL, 0);
}

/*
 * [
 * 	{
 * 		"ssid":"ssid_name",
 * 		"rssi":-40,
 * 		"enc":4
 * 	}
 * ]
 */
HTTP_STATUS serverAPI_getListOfStations(char* request, uint32_t reqSize)
{
	char payload[1000];

	SWiFiStation stationInfo[10];
	uint8_t listSize = 10;

	char* payload_ptr = &payload[1];
	payload[0] = '[';

	if(WIFI_RESP_OK != WiFi_GetAPList(stationInfo, &listSize, 5000))
	{
		return HTTP_SERVER_ERROR;
	}

	for(uint i = 0; i < listSize; i++)
	{
		payload_ptr += sprintf(payload_ptr, "{\"rssi\":%d,\"enc\":%d,\"ssid\":\"", stationInfo[i].rssi, stationInfo[i].enc);
		payload_ptr += escAndCpyStr(payload_ptr, stationInfo[i].ssid);
		if(i + 1 != listSize)
		{
			strcpy(payload_ptr, "\"},");
			payload_ptr += 3;
		}
		else
		{
			strcpy(payload_ptr, "\"}");
			payload_ptr += 2;
		}
	}

	*payload_ptr = ']';
	payload_ptr++;
	*payload_ptr = '\0';

	return sendResponse(200, payload, strlen(payload));
}

HTTP_STATUS serverAPI_getWiFiconfig(char* request, uint32_t reqSize)
{
	FIL file;
	FRESULT fResult;

	char configJsonStr[256];
	uint16_t configJsonSize = 0;

	if(FR_OK == (fResult = f_open(&file, FILE_PATH_WIFI_CONFIG, FA_READ)))
	{
		fResult = f_read(&file, configJsonStr, sizeof(configJsonStr), (UINT*)&configJsonSize);
		f_close(&file);
	}
	if(FR_OK != fResult){
		configJsonSize = 0;
		strcpy(configJsonStr, "{}");
		configJsonSize = strlen(configJsonStr);
	}

	configJsonStr[configJsonSize] = '\0';

	return sendResponse(200, configJsonStr, strlen(configJsonStr));
}

HTTP_STATUS serverAPI_setWiFiconfig(char* request, uint32_t reqSize)
{
	FIL file;
	FRESULT fResult;

	char* requestBodyPtr = NULL;
	uint16_t requestBodySize;

	char response[16];
	uint8_t responseCode = 0;

	jsmn_parser jsonParser;
	jsmntok_t jsonTokens[13];
	int16_t numOfTokens = 0;
	uint16_t tokenSize;
	char* tokenPtr;

	uint8_t dhcpEnabled = 0;
	char ssid[65] = {'\0'};
	char pass[125] = {'\0'};
	char ipAddr[16]= {'\0'};
	char maskAddr[16] = {'\0'};
	char gwAddr[16] = {'\0'};


	requestBodySize = HTTP_getContentSize(request, reqSize);
	requestBodyPtr = HTTP_getContent(request, reqSize);

	if(requestBodyPtr == NULL || ((requestBodyPtr - request + requestBodySize) != reqSize)) return HTTP_SERVER_BAD_REQUEST;

	jsmn_init(&jsonParser);

	numOfTokens = jsmn_parse(&jsonParser, requestBodyPtr, requestBodySize, jsonTokens, sizeof(jsonTokens) / sizeof(jsonTokens[0]));
	if(0 > numOfTokens || jsonTokens[0].type != JSMN_OBJECT)	//if no tokens or first token isn't object
	{
		return HTTP_SERVER_BAD_REQUEST;
	}

	for(uint16_t i = 1; i < numOfTokens; i++)
	{
		if(0 == jsmn_eq(requestBodyPtr, &jsonTokens[i], "ssid"))
		{
			tokenSize = jsonTokens[i + 1].end - jsonTokens[i + 1].start;
			tokenPtr = requestBodyPtr + jsonTokens[i + 1].start;
			uint16_t size = escStrnLen(tokenPtr, tokenSize);

			if(size > 32) return HTTP_SERVER_BAD_REQUEST;

			memcpy(ssid, tokenPtr, tokenSize);
			ssid[tokenSize] = '\0';
			i++;
		}
		else if(0 == jsmn_eq(requestBodyPtr, &jsonTokens[i], "pass"))
		{
			tokenSize = jsonTokens[i + 1].end - jsonTokens[i + 1].start;
			tokenPtr = requestBodyPtr + jsonTokens[i + 1].start;
			if(tokenSize > sizeof(pass)-1) return HTTP_SERVER_BAD_REQUEST;
			memcpy(pass, tokenPtr, tokenSize);
			pass[tokenSize] = '\0';
			i++;
		}
		else if(0 == jsmn_eq(requestBodyPtr, &jsonTokens[i], "ip"))
		{
			tokenSize = jsonTokens[i + 1].end - jsonTokens[i + 1].start;
			tokenPtr = requestBodyPtr + jsonTokens[i + 1].start;
			if(tokenSize > sizeof(ipAddr)-1) return HTTP_SERVER_BAD_REQUEST;
			memcpy(ipAddr, tokenPtr, tokenSize);
			ipAddr[tokenSize] = '\0';
			i++;
		}
		else if(0 == jsmn_eq(requestBodyPtr, &jsonTokens[i], "mask"))
		{
			tokenSize = jsonTokens[i + 1].end - jsonTokens[i + 1].start;
			tokenPtr = requestBodyPtr + jsonTokens[i + 1].start;
			if(tokenSize > sizeof(maskAddr)-1) return HTTP_SERVER_BAD_REQUEST;
			memcpy(maskAddr, tokenPtr, tokenSize);
			maskAddr[tokenSize] = '\0';
			i++;
		}
		else if(0 == jsmn_eq(requestBodyPtr, &jsonTokens[i], "gw"))
		{
			tokenSize = jsonTokens[i + 1].end - jsonTokens[i + 1].start;
			tokenPtr = requestBodyPtr + jsonTokens[i + 1].start;
			if(tokenSize > sizeof(gwAddr)-1) return HTTP_SERVER_BAD_REQUEST;
			memcpy(gwAddr, tokenPtr, tokenSize);
			gwAddr[tokenSize] = '\0';
			i++;
		}
		else if(0 == jsmn_eq(requestBodyPtr, &jsonTokens[i], "dhcp"))
		{
			tokenPtr = requestBodyPtr + jsonTokens[i + 1].start;
			dhcpEnabled = atoi(tokenPtr);
			i++;
		}
		else
		{
			return HTTP_SERVER_BAD_REQUEST;
		}
	}

	if(!dhcpEnabled)
	{
		if(!strlen(ipAddr) || !strlen(maskAddr) || !strlen(gwAddr))
		{
			return HTTP_SERVER_BAD_REQUEST;
		}
	}

	WiFi_DisconnectStation(1000);

	if(dhcpEnabled)
	{
		WiFi_EnableDHCP(1, 1000);
		WiFi_ConnectToStation(ssid, pass, &responseCode, 15000);
		WiFi_setStationAutoConnection(1, 1000);
	}
	else
	{
		if( WIFI_RESP_OK != WiFi_EnableDHCP(0, 1000) ||
			WIFI_RESP_OK != WiFi_SetIP(ipAddr, maskAddr, gwAddr, 1000))
		{
			responseCode = 5;
		}
		else
		{
			WiFi_ConnectToStation(ssid, pass, &responseCode, 15000);
			WiFi_setStationAutoConnection(1, 1000);
		}
	}

//	if(0 == responseCode)
//	{
//		if(dhcpEnabled)
//		{
//			WiFi_EnableDHCP(1, 1000);
//		}
//		else
//		{
//			if( WIFI_RESP_OK != WiFi_EnableDHCP(0, 1000) ||
//				WIFI_RESP_OK != WiFi_SetIP(ipAddr, maskAddr, gwAddr, 1000))
//			{
//				responseCode = 5;
//			}
//		}
//	}

	if(FR_OK == (fResult = f_open(&file, FILE_PATH_WIFI_CONFIG, FA_OPEN_ALWAYS | FA_WRITE)))
	{
		int res = -1;
		if(dhcpEnabled)
		{
			 res = f_printf(&file,"{\"ssid\":\"%s\",\"dhcp\":%d}", ssid, dhcpEnabled);
		}
		else
		{
			res = f_printf(&file,"{\"ssid\":\"%s\",\"ip\":\"%s\",\"mask\":\"%s\",\"gw\":\"%s\"}", ssid, ipAddr, maskAddr, gwAddr);
		}
		if(0 > res)
		{
			f_close(&file);
			return HTTP_SERVER_ERROR;
		}
		fResult = f_truncate(&file);
		f_close(&file);
	}
	if(FR_OK != fResult) return HTTP_SERVER_ERROR;


	sprintf(response, "{\"code\":%d}", responseCode);

	return sendResponse(200, response, strlen(response));
}

HTTP_STATUS serverAPI_getForecastConfig(char* request, uint32_t reqSize)
{
	FIL file;
	FRESULT fResult;

	char configJsonStr[256];
	uint16_t configJsonSize = 0;

	if(FR_OK == (fResult = f_open(&file, FILE_PATH_FORECAST_CONFIG, FA_READ)))
	{
		fResult = f_read(&file, configJsonStr, sizeof(configJsonStr), (UINT*)&configJsonSize);
		f_close(&file);
	}
	if(FR_OK != fResult){
		configJsonSize = 0;
		strcpy(configJsonStr, "{}");
		configJsonSize = strlen(configJsonStr);
	}

	configJsonStr[configJsonSize] = '\0';

	return sendResponse(200, configJsonStr, strlen(configJsonStr));
}

HTTP_STATUS serverAPI_setForecastConfig(char* request, uint32_t reqSize)
{
	FIL file;

	char configJsonStr[256];
	char* configJsonPtr = configJsonStr;

	char* requestBodyPtr = NULL;
	uint16_t requestBodySize;

	jsmn_parser jsonParser;
	jsmntok_t jsonTokens[10];
	int16_t numOfTokens = 0;
	uint16_t tokenSize;
	jsmntok_t* tokenPtr;

	bool keyIsPresent = false;

	requestBodySize = HTTP_getContentSize(request, reqSize);
	requestBodyPtr = HTTP_getContent(request, reqSize);
	if(requestBodyPtr == NULL || ((requestBodyPtr - request + requestBodySize) != reqSize)) return HTTP_SERVER_BAD_REQUEST;

	jsmn_init(&jsonParser);
	numOfTokens = jsmn_parse(&jsonParser, requestBodyPtr, requestBodySize, jsonTokens, sizeof(jsonTokens) / sizeof(jsonTokens[0]));
	if(0 > numOfTokens || jsonTokens[0].type != JSMN_OBJECT)	//if no tokens or first token isn't object
	{
		return HTTP_SERVER_BAD_REQUEST;
	}

	tokenPtr = jsmn_get_token("refresh", requestBodyPtr, jsonTokens, numOfTokens);
	if(tokenPtr == NULL){return HTTP_SERVER_BAD_REQUEST;}
	tokenSize = tokenPtr->end - tokenPtr->start;
	configJsonPtr += sprintf(configJsonPtr, "{\"refresh\":%d,", atoi(requestBodyPtr+tokenPtr->start));

	tokenPtr = jsmn_get_token("api_key", requestBodyPtr, jsonTokens, numOfTokens);
	if(tokenPtr != NULL){
		tokenSize = tokenPtr->end - tokenPtr->start;
		if(tokenSize != 32){return HTTP_SERVER_BAD_REQUEST;}
		configJsonPtr += sprintf(configJsonPtr, "\"api_key\":\"%.*s\",", tokenSize, requestBodyPtr+tokenPtr->start);
		keyIsPresent = true;
	}

	if(NULL != (tokenPtr = jsmn_get_token("country", requestBodyPtr, jsonTokens, numOfTokens)))
	{
		//If we use geocoding, then the API key is needed
		if(!keyIsPresent){return HTTP_SERVER_BAD_REQUEST;}

		tokenSize = tokenPtr->end - tokenPtr->start;
		if(tokenSize != 2){return HTTP_SERVER_BAD_REQUEST;}
		configJsonPtr += sprintf(configJsonPtr, "\"country\":\"%.*s\",", 2, requestBodyPtr+tokenPtr->start);

		tokenPtr = jsmn_get_token("zip_code", requestBodyPtr, jsonTokens, numOfTokens);
		if(tokenPtr == NULL){return HTTP_SERVER_BAD_REQUEST;}
		tokenSize = tokenPtr->end - tokenPtr->start;
		if(tokenSize > 11){return HTTP_SERVER_BAD_REQUEST;}
		configJsonPtr += sprintf(configJsonPtr, "\"zip_code\":\"%.*s\"}", tokenSize, requestBodyPtr+tokenPtr->start);
	}

	else if(NULL != (tokenPtr = jsmn_get_token("lon", requestBodyPtr, jsonTokens, numOfTokens)))
	{
		tokenSize = tokenPtr->end - tokenPtr->start;
		if(tokenSize > 10){tokenSize = 10;}
		configJsonPtr += sprintf(configJsonPtr, "\"lon\":\"%.*s\",", tokenSize, requestBodyPtr+tokenPtr->start);

		tokenPtr = jsmn_get_token("lat", requestBodyPtr, jsonTokens, numOfTokens);
		if(tokenPtr == NULL){return HTTP_SERVER_BAD_REQUEST;}
		tokenSize = tokenPtr->end - tokenPtr->start;
		if(tokenSize > 10){tokenSize = 10;}
		configJsonPtr += sprintf(configJsonPtr, "\"lat\":\"%.*s\"}", tokenSize, requestBodyPtr+tokenPtr->start);
	}
	else
	{
		return HTTP_SERVER_BAD_REQUEST;
	}

	if(FR_OK == f_open(&file, FILE_PATH_FORECAST_CONFIG, FA_OPEN_ALWAYS | FA_WRITE))
	{
		UINT writtenBytes = 0;
		if(FR_OK != f_write(&file, configJsonStr, strlen(configJsonStr), &writtenBytes) || strlen(configJsonStr) != writtenBytes )
		{
			f_close(&file);
			return HTTP_SERVER_ERROR;
		}
		f_truncate(&file);
		f_close(&file);
	}
	else
	{
		return HTTP_SERVER_ERROR;
	}

	return sendResponse(204, NULL, 0);
}


HTTP_STATUS serverAPI_getSystemConfig(char* request, uint32_t reqSize)
{
	FIL file;
	char configJsonStr[256];
	char deviceName[33] = "";
	uint8_t ledIndicator = 0;

	if(FR_OK == f_open(&file, FILE_PATH_HOSTNAME, FA_READ))
	{
		if(NULL == f_gets(deviceName, sizeof(deviceName), &file))
		{
			deviceName[0] = '\0';
		}
		f_close(&file);
	}

	if(FR_OK == f_stat(FILE_PATH_LED_IND_FLAG, NULL))
	{
		ledIndicator = 1;
	}

	sprintf(configJsonStr, "{\"device_name\":\"%s\",\"led_ind\":%d}", deviceName, ledIndicator);

	return sendResponse(200, configJsonStr, strlen(configJsonStr));
}

HTTP_STATUS serverAPI_setHostname(char* request, uint32_t reqSize)
{
	FIL file;
	jsmn_parser jsonParser;
	jsmntok_t jsonTokens[3];
	int16_t numOfTokens = 0;

	uint16_t tokenSize;
	jsmntok_t* tokenPtr;

	char* requestBodyPtr = NULL;
	uint16_t requestBodySize;

	requestBodySize = HTTP_getContentSize(request, reqSize);
	requestBodyPtr = HTTP_getContent(request, reqSize);
	if(requestBodyPtr == NULL || ((requestBodyPtr - request + requestBodySize) != reqSize)) return HTTP_SERVER_BAD_REQUEST;

	jsmn_init(&jsonParser);
	numOfTokens = jsmn_parse(&jsonParser, requestBodyPtr, requestBodySize, jsonTokens, sizeof(jsonTokens) / sizeof(jsonTokens[0]));
	if(0 > numOfTokens || jsonTokens[0].type != JSMN_OBJECT)	//if no tokens or first token isn't object
	{
		return HTTP_SERVER_BAD_REQUEST;
	}

	tokenPtr = jsmn_get_token("device_name", requestBodyPtr, jsonTokens, numOfTokens);
	if(NULL == tokenPtr) return HTTP_SERVER_BAD_REQUEST;
	tokenSize = tokenPtr->end - tokenPtr->start;

	if(tokenSize > 32) return HTTP_SERVER_BAD_REQUEST;

	if(FR_OK == f_open(&file, FILE_PATH_HOSTNAME, FA_OPEN_ALWAYS | FA_WRITE))
	{
		UINT writtenBytes = 0;
		if(FR_OK != f_write(&file, requestBodyPtr + tokenPtr->start, tokenSize, &writtenBytes) || tokenSize != writtenBytes )
		{
			f_close(&file);
			return HTTP_SERVER_ERROR;
		}
		f_truncate(&file);
		f_close(&file);
	}
	else
	{
		return HTTP_SERVER_ERROR;
	}

	return sendResponse(204, NULL, 0);
}

HTTP_STATUS serverAPI_setLedIndicator(char* request, uint32_t reqSize)
{
	FIL file;
	jsmn_parser jsonParser;
	jsmntok_t jsonTokens[3];
	int16_t numOfTokens = 0;

	jsmntok_t* tokenPtr;

	char* requestBodyPtr = NULL;
	uint16_t requestBodySize;
	uint8_t ledIndStatus = 0;

	requestBodySize = HTTP_getContentSize(request, reqSize);
	requestBodyPtr = HTTP_getContent(request, reqSize);
	if(requestBodyPtr == NULL || ((requestBodyPtr - request + requestBodySize) != reqSize)) return HTTP_SERVER_BAD_REQUEST;

	jsmn_init(&jsonParser);
	numOfTokens = jsmn_parse(&jsonParser, requestBodyPtr, requestBodySize, jsonTokens, sizeof(jsonTokens) / sizeof(jsonTokens[0]));
	if(0 > numOfTokens || jsonTokens[0].type != JSMN_OBJECT)	//if no tokens or first token isn't object
	{
		return HTTP_SERVER_BAD_REQUEST;
	}

	tokenPtr = jsmn_get_token("led_ind", requestBodyPtr, jsonTokens, numOfTokens);
	if(NULL == tokenPtr) return HTTP_SERVER_BAD_REQUEST;

	ledIndStatus = atoi(requestBodyPtr + tokenPtr->start);

	if(ledIndStatus > 0)
	{
		if(FR_OK == f_open(&file, FILE_PATH_LED_IND_FLAG, FA_OPEN_ALWAYS))
		{
			f_close(&file);
		}
		else
		{
			return HTTP_SERVER_ERROR;
		}
	}
	else
	{
		f_unlink(FILE_PATH_LED_IND_FLAG);
	}

	return sendResponse(204, NULL, 0);
}

HTTP_STATUS serverAPI_restartDevice(char* request, uint32_t reqSize, uint8_t configMode)
{
	HTTP_STATUS status = sendResponse(204, NULL, 0);
	system_sleep(1000);
	system_restart(configMode);
	return status;
}

HTTP_STATUS serverAPI_restoreDefaultSettings(char* request, uint32_t reqSize)
{
	if(0 == system_restoreDefault())
	{
		WiFi_setStationAutoConnection(0, 1000);
		return sendResponse(204, NULL, 0);
	}
	else
	{
		return sendResponse(500, NULL, 0);
	}
}

HTTP_STATUS serverAPI_voltage_test(char* request, uint32_t reqSize)
{
	char responseBuff[256];

	sprintf(responseBuff, "{\"voltage\":\"%lu\",\"bat_lvl\":%u,\"bat_chr\":%u}", system_batteryVoltage(), system_batteryLevel(), system_isCharging());

	return sendResponse(200, responseBuff, strlen(responseBuff));
}


