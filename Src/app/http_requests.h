/*
 * http_requests.h
 *
 *  Created on: 23.01.2021
 *      Author: Karol
 */

#ifndef HTTP_REQUESTS_H_
#define HTTP_REQUESTS_H_

#include <stdint.h>

typedef enum HTTP_VERSION
{
	HTTP_1_0 = 0,
	HTTP_1_1 = 1
} HTTP_VERSION;

typedef enum HTTP_METHOD
{
	HTTP_GET = 0,
	HTTP_POST,
	HTTP_UNDEF
} HTTP_METHOD;

HTTP_METHOD HTTP_getMethod(char* requestPtr, uint32_t size);
int16_t HTTP_getPath(char* requestPtr, uint32_t req_size, char* path, uint32_t path_buff_size);
int16_t HTTP_getURLParams(char* requestPtr, uint32_t req_size, char* params, uint32_t params_buff_size);
int16_t HTTP_getHeaderField(char* requestPtr, uint32_t req_size, char* headerField, char* headerValue, uint32_t valueBuffSize);
int32_t HTTP_getContentSize(char* requestPtr, uint32_t req_size);
char* HTTP_getContent(char* requestPtr, uint32_t req_size);
int16_t HTTP_getResponseCode(char* buffer, uint16_t buffer_size);
int16_t HTTP_addHeaderField(char* headerPtr, uint16_t buff_size, char* fieldName, char*fieldValue);
int16_t HTTP_createResponseHeader(char* headerPtr, uint16_t buff_size, uint16_t responseCode, uint32_t payloadSize);
int16_t HTTP_createRequestHeader(char* headerPtr, uint16_t buff_size, HTTP_METHOD method, char* path, uint32_t payloadSize);
int16_t HTTP_createRequestHeaderVer(char* headerPtr, uint16_t buff_size, HTTP_METHOD method, char* path, uint32_t payloadSize, HTTP_VERSION version);

#endif /* HTTP_REQUESTS_H_ */
