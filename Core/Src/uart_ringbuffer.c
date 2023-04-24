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
static void transmit_IT(UART_HandleTypeDef *huart);
static void endTransmit_IT(UART_HandleTypeDef *huart);

UART_RB_StatusTypeDef uart_rb_start(UART_RB_HandleTypeDef *uartRB, UART_HandleTypeDef *huart)
{
    if(uartRB == NULL || huart == NULL)
    {
        return UART_RB_WRONG_PARAMS;
    }

    if(uart_rb_isEnabled(uartRB))
    {
        return UART_RB_OK;
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
    uartRB->RB_enabled = 1;

    return UART_RB_OK;
}

UART_RB_StatusTypeDef uart_rb_stop(UART_RB_HandleTypeDef *uartRB)
{
    if(uartRB == NULL || uartRB->huart == NULL)
    {
        return UART_RB_WRONG_PARAMS;
    }

    if(!uart_rb_isEnabled(uartRB))
    {
        return UART_RB_OK;
    }

    if(HAL_OK != HAL_UART_AbortReceive(uartRB->huart))
    {
        return UART_RB_ERROR;
    }

    CLEAR_BIT(uartRB->huart->hdmarx->Instance->CCR, DMA_CCR_CIRC);      //set normal mode
    __HAL_UART_DISABLE_IT(uartRB->huart, UART_IT_IDLE);
    uartRB->RB_enabled = 0;

    return UART_RB_OK;
}

UART_RB_StatusTypeDef uart_rb_flush(UART_RB_HandleTypeDef *uartRB)
{
    UART_RB_StatusTypeDef status = UART_RB_OK;

    status = uart_rb_stop(uartRB);
    if(UART_RB_OK != status)
    {
        return status;
    }

    status = uart_rb_start(uartRB, uartRB->huart);
    return status;
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
        return writeIndex - readIndex;
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
        return UART_RB_EMPTY;
    }

    return UART_RB_OK;
}

UART_RB_StatusTypeDef uart_rb_getNextChar(UART_RB_HandleTypeDef *uartRB, char* charBuffer)
{
    uint16_t readIndex;
    int32_t dataSize;

    if(uartRB == NULL)
    {
        return UART_RB_WRONG_PARAMS;
    }

    dataSize = uart_rb_dataLen(uartRB);

    if(dataSize == 0)
    {
        return UART_RB_EMPTY;
    }

    readIndex = CIRCULAR_INC(uartRB->RB_readIndex);
    *charBuffer = (char)uartRB->UART_Buffer[readIndex];

    if(uartRB->RB_overflowFlag)
    {
        *charBuffer = '\0';
        return UART_RB_OVERFLOW;
    }

    uartRB->RB_readIndex = readIndex;

    return UART_RB_OK;
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
        transmit_IT(huart);
        return;
    }

    /* UART in mode Transmitter end --------------------------------------------*/
    if (((isrflags & USART_SR_TC) != RESET) && ((cr1its & USART_CR1_TCIE) != RESET))
    {
        endTransmit_IT(huart);
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
    int32_t readIndex = uartRB->RB_readIndex;
    int32_t oldWriteIndex = uartRB->RB_writeIndex;
    int32_t newWriteIndex = UART_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(hdma);

    if(0 > ((oldWriteIndex - newWriteIndex) * (readIndex - oldWriteIndex) * (newWriteIndex - readIndex)))
    {
        uart_rb_stop(uartRB);
        uartRB->RB_overflowFlag = 1;
    }
    uartRB->RB_writeIndex = newWriteIndex;
}

/*
 * This function is copy of UART_Transmit_IT from HAL lib.
 */
static void transmit_IT(UART_HandleTypeDef *huart)
{
    uint16_t *tmp;

    /* Check that a Tx process is ongoing */
    if (huart->gState != HAL_UART_STATE_BUSY_TX)
    {
        return;
    }

    if ((huart->Init.WordLength == UART_WORDLENGTH_9B) && (huart->Init.Parity == UART_PARITY_NONE))
    {
        tmp = (uint16_t *) huart->pTxBuffPtr;
        huart->Instance->DR = (uint16_t)(*tmp & (uint16_t)0x01FF);
        huart->pTxBuffPtr += 2U;
    }
    else
    {
        huart->Instance->DR = (uint8_t)(*huart->pTxBuffPtr++ & (uint8_t)0x00FF);
    }

    if (--huart->TxXferCount == 0U)
    {
        /* Disable the UART Transmit Complete Interrupt */
        __HAL_UART_DISABLE_IT(huart, UART_IT_TXE);

        /* Enable the UART Transmit Complete Interrupt */
        __HAL_UART_ENABLE_IT(huart, UART_IT_TC);
    }
    return;
}

/*
 * This function is copy of UART_EndTransmit_IT from HAL lib.
 */
static void endTransmit_IT(UART_HandleTypeDef *huart)
{
    /* Disable the UART Transmit Complete Interrupt */
    __HAL_UART_DISABLE_IT(huart, UART_IT_TC);

    /* Tx process is ended, restore huart->gState to Ready */
    huart->gState = HAL_UART_STATE_READY;

#if (USE_HAL_UART_REGISTER_CALLBACKS == 1)
    /*Call registered Tx complete callback*/
    huart->TxCpltCallback(huart);
#else
    /*Call legacy weak Tx complete callback*/
    HAL_UART_TxCpltCallback(huart);
#endif /* USE_HAL_UART_REGISTER_CALLBACKS */

    return;
}

