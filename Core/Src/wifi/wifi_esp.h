/*
 * wifi_esp.h
 *
 *  Created on: 06.01.2021
 *      Author: Karol
 */

#ifndef WIFI_ESP_H_
#define WIFI_ESP_H_

#include "stdbool.h"
#include "usart.h"
#include <time.h>

#define SNTP_ADDRESS_1	"0.pl.pool.ntp.org"
#define SNTP_ADDRESS_2	"0.europe.pool.ntp.org"
#define SNTP_ADDRESS_3	"0.pool.ntp.org"

#define WiFiUart huart1
#define WiFi_RST_GPIO	WIFI_RST_GPIO_Port
#define WiFi_RST_PIN	WIFI_RST_Pin
#define WiFi_PWR_GPIO	WIFI_PWR_GPIO_Port
#define WiFi_PWR_PIN	WIFI_PWR_Pin

typedef enum Wifi_RespStatus
{
	WIFI_RESP_OK = 0,
	WIFI_RESP_ERROR,
	WIFI_RESP_TIMEOUT
} Wifi_RespStatus;

typedef enum Wifi_LinkStatus
{
	WIFI_LINK_OK = 0,
	WIFI_LINK_ALREADY_CONNECT,
	WIFI_LINK_ERROR,
	WIFI_LINK_NO_IP,
	WIFI_LINK_DNS_FAIL
} Wifi_LinkStatus;

typedef enum WiFi_AP_ConnResp
{
	WIFI_AP_CONN_OK = 0,
	WIFI_AP_CONN_TIMEOUT,
	WIFI_AP_CONN_WRONG_PASS,
	WIFI_AP_CONN_AP_LOST,
	WIFI_AP_CONN_FAILED
} WiFi_AP_ConnResp;

typedef enum WiFi_Encryption
{
	WIFI_ENC_OPEN = 0,
	WIFI_ENC_WEP,
	WIFI_ENC_WPA_PSK,
	WIFI_ENC_WPA2_PSK,
	WIFI_ENC_WPA_WPA2_PSK,
	WIFI_ENC_NUM
} WiFi_Encryption;

typedef struct SWiFi_AP_Config
{
	char ssid[65];
	char pass[128];
	uint8_t maxConnection;
	uint8_t channel;
	WiFi_Encryption enc;
} SWiFi_AP_Config;

typedef struct SWiFiLinkStatus
{
	bool connected;
	int16_t dataWaiting;
} SWiFiLinkStatus;

typedef struct SWiFiStatus{
	bool connectedToAP;
	SWiFiLinkStatus linksStatus[5];
} SWiFiStatus;

typedef struct SWiFiStation
{
	WiFi_Encryption enc;
	int16_t rssi;
	char ssid[33];
} SWiFiStation;

extern volatile SWiFiStatus wifiStatus;


Wifi_RespStatus WiFi_restart(uint32_t timeout);
void WiFi_shutdown(void);
Wifi_RespStatus WiFi_MessageReceivingStart(bool loopMode);
Wifi_RespStatus WiFi_MessageReceivingStop(void);
Wifi_RespStatus WiFi_UpdateStatus(const uint32_t timeout);
Wifi_RespStatus WiFi_AP_mode(SWiFi_AP_Config* ap_conf);
Wifi_RespStatus WiFi_OpenServerSocket(uint16_t port, uint8_t maxConn, uint16_t serverTimeout, uint32_t timeout);
Wifi_RespStatus WiFi_CloseServerSocket(uint16_t port);
Wifi_RespStatus WiFi_HandleMessage(char* message, uint32_t size);
int32_t WiFi_ReadData(uint8_t linkID, uint8_t* data, uint16_t dataSize, uint32_t readData);
Wifi_RespStatus WiFi_SendData(uint8_t linkID, uint8_t* data, uint16_t dataSize);
Wifi_RespStatus WiFi_SendDataEx(uint8_t linkID, uint8_t* data, uint16_t dataSize, bool wait);

int8_t WiFi_OpenTCPConnection(char * address, uint16_t port, uint16_t keepAlive);
Wifi_RespStatus WiFi_CloseConnection(uint8_t linkID);
bool WiFi_MessageReceivingIsRunning(void);
Wifi_RespStatus WiFi_GetVersionString(char* versionStringPtr, uint16_t size, uint32_t timeout);

Wifi_RespStatus WiFi_ConnectToStation(char* ssid, char* pass, uint8_t* connectStatus, uint32_t timeout);
Wifi_RespStatus WiFi_DisconnectStation(uint32_t timeout);
Wifi_RespStatus WiFi_setStationAutoConnection(uint8_t enable, uint32_t timeout);

Wifi_RespStatus WiFi_EnableDHCP(uint8_t dhcpEnable, uint32_t timeout);
Wifi_RespStatus WiFi_SetIP(char* ip, char* mask, char* gateway, uint32_t timeout);
Wifi_RespStatus WiFi_getIPAddress(char* ip, uint32_t timeout);

Wifi_RespStatus WiFi_GetAPList(SWiFiStation* stationlist, uint8_t* listSize, uint32_t timeout);
Wifi_RespStatus WiFi_setStationName(char* name, uint32_t timeout);

Wifi_RespStatus WiFi_setSNTPconfig(uint8_t enable, int8_t timezone, uint32_t timeout);
Wifi_RespStatus WiFi_getSNTPtime(time_t* timeSecPtr, uint32_t timeout);

bool WiFi_SendingComplete(void);
#endif /* WIFI_ESP_H_ */
