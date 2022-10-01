/*
 * http_request.c
 *
 *  Created on: 23.01.2021
 *      Author: Karol
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <http_requests.h>

HTTP_METHOD HTTP_getMethod(char* requestPtr, uint32_t size)
{
	if(0 == strncmp("GET", requestPtr, 3))
	{
		return HTTP_GET;
	}
	else if(0 == strncmp("POST", requestPtr, 4))
	{
		return HTTP_POST;
	}
	else
	{
		return HTTP_UNDEF;
	}
}

int16_t HTTP_getURLParams(char* requestPtr, uint32_t req_size, char* params, uint32_t params_buff_size)
{
	char* paramsStartPtr = NULL;
	char* paramsEndPtr = NULL;
	char* requestEnd = requestPtr + req_size;
	uint16_t paramsSize = 0;

	for(; requestPtr < requestEnd; requestPtr++)
	{
		if(*(requestPtr) == '\r' || *(requestPtr) == '\n')
		{
			return -1;
		}
		else if(paramsStartPtr == NULL && *(requestPtr) == '?')
		{
			paramsStartPtr = requestPtr + 1;
		}
		else if(paramsStartPtr != NULL && *(requestPtr) == ' ')
		{
			paramsEndPtr = requestPtr;
			break;
		}
	}

	if(paramsStartPtr == NULL || paramsEndPtr == NULL)
	{
		return -2;
	}

	paramsSize = paramsEndPtr - paramsStartPtr;
	if(params_buff_size <= paramsSize)
	{
		return -3;
	}

	memcpy(params, paramsStartPtr, paramsSize);
	*(params+paramsSize) = '\0';
	return paramsSize;

}

int16_t HTTP_getPath(char* requestPtr, uint32_t req_size, char* path, uint32_t path_buff_size)
{
	char* pathStartPtr = NULL;
	char* pathEndPtr = NULL;
	char* requestEnd = requestPtr + req_size;
	uint16_t pathSize = 0;

	for(; requestPtr < requestEnd; requestPtr++)
	{
		if(*(requestPtr) == '\r' || *(requestPtr) == '\n')
		{
			return -1;
		}
		else if(*(requestPtr) == ' ' || *(requestPtr) == '?')
		{
			if(pathStartPtr == NULL)
			{
				pathStartPtr = requestPtr + 1;
			}
			else
			{
				pathEndPtr = requestPtr;
				break;
			}
		}
	}

	if(pathStartPtr == NULL || pathEndPtr == NULL)
	{
		return -2;
	}

	pathSize = pathEndPtr - pathStartPtr;
	if(path_buff_size <= pathSize)
	{
		return -3;
	}

	memcpy(path, pathStartPtr, pathSize);
	*(path+pathSize) = '\0';
	return pathSize;
}

int16_t HTTP_getHeaderField(char* requestPtr, uint32_t req_size, char* headerField, char* headerValue, uint32_t valueBuffSize)
{
	char* requestEnd = requestPtr + req_size;
	const uint16_t headerFieldSize = strlen(headerField);

	while(requestPtr < requestEnd)
	{
		if(0 == strncmp(headerField, requestPtr, headerFieldSize))
		{
			requestPtr += headerFieldSize + 2;
			char* startValPtr =  requestPtr;	//<field>: <value>
			uint16_t valueSize = 0;

			while(*requestPtr != '\r')
			{
				requestPtr++;
				if(requestPtr >= requestEnd)
				{
					return -1;
				}
			}

			valueSize = requestPtr - startValPtr;
			if(valueBuffSize <= valueSize)
			{
				memcpy(headerValue, startValPtr, valueBuffSize-1);
				*(headerValue + (valueBuffSize-1)) = '\0';
			}
			else
			{
				memcpy(headerValue, startValPtr, valueSize);
				*(headerValue + valueSize) = '\0';
			}


			return valueSize;
		}
		else if(0 == strncmp("\r\n", requestPtr, 4))
		{
			//end of header
			return -1;
		}

		while(*requestPtr != '\n')
		{
			requestPtr++;
			if(requestPtr >= requestEnd)
			{
				return -1;
			}
		}
		requestPtr++;
	}
	return -1;
}

int32_t HTTP_getContentSize(char* requestPtr, uint32_t req_size)
{
	char contentSizeStr[6];
	if(0 < HTTP_getHeaderField(requestPtr, req_size, "Content-Length", contentSizeStr, sizeof(contentSizeStr)))
	{
		return atoi(contentSizeStr);
	}
	else
	{
		return -1;
	}
}

char* HTTP_getContent(char* requestPtr, uint32_t req_size)
{
	const char endHeaderPattern[] = {'\r', '\n', '\r', '\n'};
	const char* endRequestPointer = requestPtr + req_size;
	uint8_t patternCounter = 0;

	while(requestPtr < endRequestPointer)
	{
		if(*requestPtr == endHeaderPattern[patternCounter])
		{
			patternCounter++;
			if(patternCounter == sizeof(endHeaderPattern))
			{
				return requestPtr + 1;
			}
		}
		else
		{
			patternCounter = 0;
		}
		requestPtr++;
	}
	return NULL;
}

int16_t HTTP_getResponseCode(char* buffer, uint16_t buffer_size)
{
	if(buffer_size < 13)
	{
		return -1;
	}
	return atoi(buffer + 8);	//HTTP/1.1 200 OK
}


int16_t HTTP_addHeaderField(char* headerPtr, uint16_t buff_size, char* fieldName, char*fieldValue)
{
	char* endHeader = HTTP_getContent(headerPtr, buff_size);
	uint16_t oldHeaderSize = 0;

	if(endHeader == NULL)
	{
		return -1;
	}
	oldHeaderSize = (uint16_t)(endHeader-headerPtr);

	if((buff_size-oldHeaderSize) <= (strlen(fieldName) + strlen(fieldValue) + 5))	//<fieldName>: <fieldValue>\r\n + \0
	{
		return -1;
	}

	headerPtr += (oldHeaderSize - 2);

	oldHeaderSize += sprintf(headerPtr, "%s: %s\r\n\r\n", fieldName, fieldValue);
	return (oldHeaderSize - 2);
}

int16_t HTTP_createResponseHeader(char* headerPtr, uint16_t buff_size, uint16_t responseCode, uint32_t payloadSize)
{
	uint16_t size = 0;
	char* responseDescription = NULL;
	switch (responseCode)
	{
		case 200:
			responseDescription = "OK";
			break;
		case 400:
			responseDescription = "Bad Request";
			break;
		case 401:
			responseDescription = "Unauthorized";
			break;
		case 404:
			responseDescription = "Not Found";
			break;
		case 500:
			responseDescription = "Internal Server Error";
			break;
		case 501:
			responseDescription = "Not Implemented";
			break;
		default:
			responseDescription = "";
	}

	size = snprintf(headerPtr, buff_size, "HTTP/1.1 %d %s\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n", responseCode, responseDescription, payloadSize);

	if(size >= buff_size - 1)
	{
		return -1;
	}

	return size;
}

int16_t HTTP_createRequestHeader(char* headerPtr, uint16_t buff_size, HTTP_METHOD method, char* path, uint32_t payloadSize)
{
	uint16_t size = 0;
	switch (method)
	{
		case HTTP_GET:
			size += snprintf(headerPtr, buff_size, "GET %s HTTP/1.1\r\n\r\n", path);
			break;
		case HTTP_POST:
			size += snprintf(headerPtr, buff_size, "POST %s HTTP/1.1\r\nContent-Length: %ld\r\n\r\n", path, payloadSize);
			break;
		default:
			return -1;
			break;
	}

	if(size >= buff_size - 1)
	{
		return -1;
	}
	return size;
}
