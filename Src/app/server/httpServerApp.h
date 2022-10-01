/*
 * httpServerApp.h
 *
 *  Created on: 24.01.2021
 *      Author: Karol
 */

#ifndef HTTPSERVERAPP_H_
#define HTTPSERVERAPP_H_

#include <stdint.h>

#define HTTP_SERVER_REQUEST_SIZE      1000
#define HTTP_SERVER_RESPONSE_SIZE     2048
#define HTTP_SERVER_REQUEST_PATH_MAX_SIZE (64)
#define HTTP_SERVER_STATIC_PATH       "/html"

uint8_t runServerApp(uint16_t port, uint8_t maxConnection, uint16_t serverTimeout);

#endif /* HTTPSERVERAPP_H_ */
