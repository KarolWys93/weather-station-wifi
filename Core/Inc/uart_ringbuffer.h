/*
 * uart_ringbuffer.h
 *
 *  Created on: Apr 22, 2023
 *      Author: Karol
 */

#ifndef INC_UART_RINGBUFFER_H_
#define INC_UART_RINGBUFFER_H_

#include "usart.h"
#include "dma.h"

#define UART_BUFFER_SIZE    128

typedef enum
{
    UART_RB_OK           = 0x00U,
    UART_RB_EMPTY        = 0x01U,
    UART_RB_OVERFLOW     = 0x02U,
    UART_RB_WRONG_PARAMS = 0x03U,
    UART_RB_ERROR        = 0x04U
} UART_RB_StatusTypeDef;

typedef struct
{
    UART_HandleTypeDef* huart;
    uint16_t RB_readIndex;
    uint16_t RB_writeIndex;
    uint16_t RB_enabled;
    uint16_t RB_overflowFlag;
    uint8_t UART_Buffer[UART_BUFFER_SIZE];
} UART_RB_HandleTypeDef;


#define uart_rb_isOverflow(uartRB_ptr) ((uartRB_ptr)->RB_overflowFlag)
#define uart_rb_isEnabled(uartRB_ptr) ((uartRB_ptr)->RB_enabled)


UART_RB_StatusTypeDef uart_rb_start(UART_RB_HandleTypeDef *uartRB, UART_HandleTypeDef *huart);
UART_RB_StatusTypeDef uart_rb_stop(UART_RB_HandleTypeDef *uartRB);

UART_RB_StatusTypeDef uart_rb_flush(UART_RB_HandleTypeDef *uartRB);

UART_RB_StatusTypeDef uart_rb_getNextLine(UART_RB_HandleTypeDef *uartRB, char* lineBuffer, uint16_t buffSize, uint16_t *rb);
UART_RB_StatusTypeDef uart_rb_getNextChar(UART_RB_HandleTypeDef *uartRB, char* charBuffer);

int32_t uart_rb_dataLen(UART_RB_HandleTypeDef *uartRB);

void uart_rb_uart_irq(UART_RB_HandleTypeDef *uartRB);
void uart_rb_dma_irq(UART_RB_HandleTypeDef *uartRB);


#endif /* INC_UART_RINGBUFFER_H_ */
