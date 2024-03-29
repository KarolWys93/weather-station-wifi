/*
 * httpServer_api.h
 *
 *  Created on: 14.02.2021
 *      Author: Karol
 */

#ifndef HTTPSERVER_API_H_
#define HTTPSERVER_API_H_

#include "httpServer_internal.h"

HTTP_STATUS serverAPI_status(char* request, uint32_t reqSize);
HTTP_STATUS serverAPI_getAPconfig(char* request, uint32_t reqSize);
HTTP_STATUS serverAPI_setAPconfig(char* request, uint32_t reqSize);
HTTP_STATUS serverAPI_getListOfStations(char* request, uint32_t reqSize);
HTTP_STATUS serverAPI_getWiFiconfig(char* request, uint32_t reqSize);
HTTP_STATUS serverAPI_setWiFiconfig(char* request, uint32_t reqSize);
HTTP_STATUS serverAPI_getForecastConfig(char* request, uint32_t reqSize);
HTTP_STATUS serverAPI_getSystemConfig(char* request, uint32_t reqSize);
HTTP_STATUS serverAPI_setForecastConfig(char* request, uint32_t reqSize);
HTTP_STATUS serverAPI_setHostname(char* request, uint32_t reqSize);
HTTP_STATUS serverAPI_setLedIndicator(char* request, uint32_t reqSize);
HTTP_STATUS serverAPI_restartDevice(char* request, uint32_t reqSize, uint8_t configMode);
HTTP_STATUS serverAPI_restoreDefaultSettings(char* request, uint32_t reqSize);

//test&debug API
HTTP_STATUS serverAPI_logs(char* request, uint32_t reqSize);
HTTP_STATUS serverAPI_voltage(char* request, uint32_t reqSize);
HTTP_STATUS serverAPI_ledTest(char* request, uint32_t reqSize);
HTTP_STATUS serverAPI_imgTest(char* request, uint32_t reqSize);


#endif /* HTTPSERVER_API_H_ */
