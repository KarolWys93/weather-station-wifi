/*
 * uart_ringbuffer.c
 *
 *  Created on: Apr 22, 2023
 *      Author: Karol
 */

#include "uart_ringbuffer.h"

#define CIRCULAR_INC(value) ((value + 1) % UART_BUFFER_SIZE)

static const uint8_t newLinePattern[] = {'\r', '\n'};
#define PATTERN_SIZE (sizeof(newLinePattern))


extern HAL_StatusTypeDef UART_Transmit_IT(UART_HandleTypeDef *huart);
extern HAL_StatusTypeDef UART_EndTransmit_IT(UART_HandleTypeDef *huart);


static void uart_rb_irq(UART_RB_HandleTypeDef *uartRB);

UART_RB_StatusTypeDef uart_rb_start(UART_RB_HandleTypeDef *uartRB, UART_HandleTypeDef *huart)
{
    if(uartRB == NULL || huart == NULL)
    {
        return UART_RB_WRONG_PARAMS;
    }

    uartRB->huart = huart;

    SET_BIT(uartRB->huart->hdmarx->Instance->CCR, DMA_CCR_CIRC);            //set circular mode

    uartRB->RB_readIndex = UART_BUFFER_SIZE - 1;
    uartRB->RB_writeIndex = 0;
    uartRB->RB_overflowFlag = 0;

    __HAL_DMA_ENABLE_IT(uartRB->huart->hdmarx, (DMA_IT_TC | DMA_IT_HT));    // UART DMA TC & HC interrupt
    __HAL_UART_ENABLE_IT(uartRB->huart, UART_IT_IDLE);                      // UART Idle Line interrupt

    if(HAL_OK != HAL_UART_Receive_DMA(uartRB->huart, uartRB->UART_Buffer, UART_BUFFER_SIZE))
    {
        return UART_RB_ERROR;
    }

    return UART_RB_OK;
}

UART_RB_StatusTypeDef uart_rb_stop(UART_RB_HandleTypeDef *uartRB)
{
    if(uartRB == NULL || uartRB->huart == NULL)
    {
        return UART_RB_WRONG_PARAMS;
    }

    if(HAL_OK != HAL_DMA_Abort(uartRB->huart->hdmarx))
    {
        return UART_RB_ERROR;
    }

    CLEAR_BIT(uartRB->huart->hdmarx->Instance->CCR, DMA_CCR_CIRC);      //set normal mode
    __HAL_UART_DISABLE_IT(uartRB->huart, UART_IT_IDLE);

    return UART_RB_OK;
}

int32_t uart_rb_dataLen(UART_RB_HandleTypeDef *uartRB)
{
    uint16_t writeIndex;
    uint16_t readIndex;

    if(uartRB->RB_overflowFlag)
    {
        return -1;
    }

    writeIndex = uartRB->RB_writeIndex;
    readIndex = CIRCULAR_INC(uartRB->RB_readIndex);

    if(writeIndex >= readIndex)
    {
        return readIndex - writeIndex;
    }
    else
    {
        return UART_BUFFER_SIZE + writeIndex - readIndex;
    }
}

UART_RB_StatusTypeDef uart_rb_getNextLine(UART_RB_HandleTypeDef *uartRB, char* lineBuffer, uint16_t buffSize, uint16_t *rb)
{
    uint16_t readIndex;
    int32_t dataSize;
    uint16_t readDataCnt = 0;
    uint8_t newLineReady = 0;
    uint8_t patternCounter = 0;

    if(uartRB == NULL || lineBuffer == NULL || buffSize == 0)
    {
        return UART_RB_WRONG_PARAMS;
    }

    lineBuffer[0] = '\0';
    dataSize = uart_rb_dataLen(uartRB);

    if(dataSize == 0)
    {
        return UART_RB_EMPTY;
    }

    readIndex = CIRCULAR_INC(uartRB->RB_readIndex);

    while(readDataCnt < dataSize)
    {
        char currentChar = (char)uartRB->UART_Buffer[readIndex];

        if(readDataCnt < buffSize - 1)  //the last position should be free
        {
            lineBuffer[readDataCnt] = currentChar;
        }

        patternCounter = (currentChar == newLinePattern[patternCounter])? (patternCounter + 1) : 0;
        if(patternCounter >= PATTERN_SIZE)
        {
            newLineReady = 1;
            break;
        }

        readDataCnt++;
        readIndex = CIRCULAR_INC(readIndex);
    }

    if(uartRB->RB_overflowFlag)
    {
        return UART_RB_OVERFLOW;
    }

    if(newLineReady)
    {
        uartRB->RB_readIndex = readIndex;
        if(readDataCnt < buffSize)  //output buffer is not smaller than line
        {
            readDataCnt -= (PATTERN_SIZE - 1);
            lineBuffer[readDataCnt] = '\0';
            *rb = readDataCnt;
        }
        else
        {
            lineBuffer[buffSize - 1] = '\0';
            *rb = readDataCnt - (PATTERN_SIZE - 1);
        }
    }
    else
    {
        lineBuffer[0] = '\0';
        *rb = 0;
    }

    return UART_RB_OK;
}

uint8_t uart_rb_isOverflow(UART_RB_HandleTypeDef *uartRB)
{
    return uartRB->RB_overflowFlag;
}


void uart_rb_uart_irq(UART_RB_HandleTypeDef *uartRB)
{
    UART_HandleTypeDef* huart = uartRB->huart;
    uint32_t isrflags   = READ_REG(huart->Instance->SR);
    uint32_t cr1its     = READ_REG(huart->Instance->CR1);

    if(((isrflags & USART_SR_IDLE) != 0U) && ((cr1its & USART_SR_IDLE) != 0U))
    {
        __HAL_UART_CLEAR_IDLEFLAG(huart);
        uart_rb_irq(uartRB);

        return;
    }

    // Transmit parts works in the same way as in HAL lib.

    /* UART in mode Transmitter ------------------------------------------------*/
    if (((isrflags & USART_SR_TXE) != RESET) && ((cr1its & USART_CR1_TXEIE) != RESET))
    {
        UART_Transmit_IT(huart);
        return;
    }

    /* UART in mode Transmitter end --------------------------------------------*/
    if (((isrflags & USART_SR_TC) != RESET) && ((cr1its & USART_CR1_TCIE) != RESET))
    {
        UART_EndTransmit_IT(huart);
        return;
    }
}

void uart_rb_dma_irq(UART_RB_HandleTypeDef *uartRB)
{
    DMA_HandleTypeDef *hdma = uartRB->huart->hdmarx;

    uint32_t flag_it = hdma->DmaBaseAddress->ISR;
    uint32_t source_it = hdma->Instance->CCR;

    if (((flag_it & (DMA_FLAG_HT1 << hdma->ChannelIndex)) != RESET) && ((source_it & DMA_IT_HT) != RESET))
    {
        __HAL_DMA_CLEAR_FLAG(hdma, __HAL_DMA_GET_HT_FLAG_INDEX(hdma));

        uart_rb_irq(uartRB);
    }
    else if (((flag_it & (DMA_FLAG_TC1 << hdma->ChannelIndex)) != RESET) && ((source_it & DMA_IT_TC) != RESET))
    {
        __HAL_DMA_CLEAR_FLAG(hdma, __HAL_DMA_GET_TC_FLAG_INDEX(hdma));
        uart_rb_irq(uartRB);
    }
}

static void uart_rb_irq(UART_RB_HandleTypeDef *uartRB)
{
    DMA_HandleTypeDef *hdma = uartRB->huart->hdmarx;
    uint16_t readIndex = uartRB->RB_readIndex;
    uint16_t oldWriteIndex = uartRB->RB_writeIndex;
    uint16_t newWriteIndex = UART_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(hdma);

    if(0 > ((oldWriteIndex - newWriteIndex) * (readIndex - oldWriteIndex) * (newWriteIndex - readIndex)))
    {
        uart_rb_stop(uartRB);
        uartRB->RB_overflowFlag = 1;
    }
    uartRB->RB_writeIndex = newWriteIndex;
}
