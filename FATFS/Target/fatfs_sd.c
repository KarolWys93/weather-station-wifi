/*
 * fatfs_sd.c
 *
 *  Created on: 27.12.2020
 *      Author: Karol
 */


#define TRUE  1
#define FALSE 0
#define bool BYTE

#include "gpio.h"
#include "spi.h"
#include "dma.h"
#include "diskio.h"
#include "fatfs_sd.h"


static volatile DSTATUS Stat = STA_NOINIT;              /* Disk Status Flag*/
static uint8_t CardType;                                /* SD type 0: MMC, 1: SDC, 2: Block addressing*/
static uint8_t PowerFlag = 0;                           /* Power Status Flag */

/* SPI Chip Select */
static void SELECT(void)
{
    HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);
}

/* SPI Chip Deselect */
static void DESELECT(void)
{
    HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
}


/* SPI data transfer */
static void SPI_TxMultiByte(const BYTE *data, DWORD dataSize)
{
    HAL_SPI_Transmit_IT(&hspi2, (uint8_t *)data, dataSize);
    while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY)
    {
        HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
    }
}

static void SPI_TxByte(BYTE data)
{
    SPI_TxMultiByte(&data, 1);
}

static uint8_t SPI_RxByte(void)
{
    uint8_t dummy, data;
    dummy = 0xFF;
    data = 0;

    HAL_SPI_TransmitReceive_IT(&hspi2, &dummy, &data, 1);
    while (HAL_SPI_GetState(&hspi2) != HAL_SPI_STATE_READY)
    {
        HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
    }

    return data;
}

static void SPI_RxMultiByte(BYTE *data, DWORD dataSize)
{
    for(uint32_t i = 0; i < dataSize; i++)
    {
        *(data + i) = SPI_RxByte();
    }
}

/* Standby SD card Ready */
static uint8_t SD_ReadyWait(void)
{
    uint8_t res;

    /* 500 ms counter ready */
    uint32_t timer2 = HAL_GetTick() + 500;
    SPI_RxByte();

    do
    {
        /* SPI communication until a value of 0xFF is received */
        res = SPI_RxByte();
    } while ((res != 0xFF) && (timer2 > HAL_GetTick()));

    return res;
}

/*Power on*/
static void SD_PowerOn(void)
{
    uint8_t cmd_arg[6];
    uint32_t Count = 0x1FFF;

    /* In the Deselect state, the SPI message is transmitted to the standby state. */
    DESELECT();

    HAL_GPIO_WritePin(SD_PWR_GPIO_Port, SD_PWR_Pin, GPIO_PIN_SET);
    system_sleep(200);

    for(int i = 0; i < 10; i++)
    {
        SPI_TxByte(0xFF);
    }

    /* SPI Chips Select */
    SELECT();

    /* Initial GO_IDLE_STATE state transition */
    cmd_arg[0] = (CMD0 | 0x40);
    cmd_arg[1] = 0;
    cmd_arg[2] = 0;
    cmd_arg[3] = 0;
    cmd_arg[4] = 0;
    cmd_arg[5] = 0x95;


    /* Command transmission */
    SPI_TxMultiByte(cmd_arg, 6);

    /* Answer waiting*/
    while ((SPI_RxByte() != 0x01) && Count)
    {
        Count--;
    }

    DESELECT();
    SPI_TxByte(0XFF);

    PowerFlag = 1;
}

/*
Power off */
static void SD_PowerOff(void)
{
    HAL_GPIO_WritePin(SD_PWR_GPIO_Port, SD_PWR_Pin, GPIO_PIN_RESET);
    PowerFlag = 0;
}

/* Check power status*/
static uint8_t SD_CheckPower(void)
{
    /*  0=off, 1=on */
    return PowerFlag;
}

/*Receive data packet */
static bool SD_RxDataBlock(BYTE *buff, UINT btr)
{
    uint8_t token;

    /* 100ms timer */
    uint32_t timer1 = HAL_GetTick() + 100;

    /*
Answer waiting */
    do
    {
        token = SPI_RxByte();
    } while((token == 0xFF) && (timer1 > HAL_GetTick()));

    /* Error processing when receiving Token other than 0xFE */
    if(token != 0xFE)
        return FALSE;

    /*
Receiving data into a buffer */
    SPI_RxMultiByte(buff, btr);


    SPI_RxByte(); /* Ignore CRC*/
    SPI_RxByte();

    return TRUE;
}

/*Data transmission packet */
#if _READONLY == 0
static bool SD_TxDataBlock(const BYTE *buff, BYTE token)
{
    uint8_t resp = 0xFF;
    uint8_t i = 0;

    /* Waiting for SD card preparation */
    if (SD_ReadyWait() != 0xFF)
        return FALSE;

    /*Token transfer */
    SPI_TxMultiByte(&token, 1);

    /* For data token */
    if (token != 0xFD)
    {

        /* 512 byte data transmission */
        SPI_TxMultiByte(buff, 512);

        SPI_RxByte();       /*Ignore CRC */
        SPI_RxByte();

        /* Receive Date Response*/
        while (i <= 64)
        {
            resp = SPI_RxByte();

            /* Error response processing */
            if ((resp & 0x1F) == 0x05)
                break;

            i++;
        }

        /*SPI receive buffer clear*/
        while (SPI_RxByte() == 0);
    }

    if ((resp & 0x1F) == 0x05)
        return TRUE;
    else
        return FALSE;
}
#endif /* _READONLY */

/* CMD packet transmission */
static BYTE SD_SendCmd(BYTE cmd, DWORD arg)
{
    uint8_t crc, res;
    uint8_t argArr[4];

    /*SD card standby */
    if (SD_ReadyWait() != 0xFF)
        return 0xFF;

    argArr[0] = (BYTE) (arg >> 24);   /* Argument[31..24] */
    argArr[1] = (BYTE) (arg >> 16);   /* Argument[23..16] */
    argArr[2] = (BYTE) (arg >> 8);    /* Argument[15..8] */
    argArr[3] = (BYTE) arg;           /* Argument[7..0] */

    /* Command packet transmission */
    SPI_TxMultiByte(&cmd, 1);			/* Command */
    SPI_TxMultiByte(argArr, 4);

    /*
Prepare CRC by command */
    crc = 0;
    if (cmd == CMD0)
        crc = 0x95; /* CRC for CMD0(0) */

    if (cmd == CMD8)
        crc = 0x87; /* CRC for CMD8(0x1AA) */

    /* CRC transmission */
    SPI_TxMultiByte(&crc, 1);

    /*
For the CMD12 Stop Reading command, discard one response byte */
    if (cmd == CMD12)
        SPI_RxByte();

    /* And receives normal data within 10 times. */
    uint8_t n = 10;
    do
    {
        res = SPI_RxByte();
    } while ((res & 0x80) && --n);

    return res;
}

/*-----------------------------------------------------------------------

Global functions used in fatfs
  Used in the user_diskio.c file.
-----------------------------------------------------------------------*/

/* SD card initialization */
DSTATUS SD_disk_initialize(BYTE drv)
{
    uint8_t type, ocr[4];

    /*
Only one type of drive is supported*/
    if(drv)
        return STA_NOINIT;

    /*No SD card inserted */
    if(Stat & STA_NODISK)
        return Stat;

    /* SD card Power On */
    SD_PowerOn();

    /*For SPI communication Chip Select */
    SELECT();

    /* SD card type variable initialization*/
    type = 0;

    /* Idle state entry */
    if (SD_SendCmd(CMD0, 0) == 1)
    {
        /*Timer 1 second set */
        uint32_t timer1 = HAL_GetTick() + 1000;

        /* Confirm SD interface operation condition*/
        if (SD_SendCmd(CMD8, 0x1AA) == 1)
        {
            /* SDC Ver2+ */
            SPI_RxMultiByte(ocr, 4);

            if (ocr[2] == 0x01 && ocr[3] == 0xAA)
            {
                /* 2.7-3.6V Voltage Range Operation */
                do {
                    if (SD_SendCmd(CMD55, 0) <= 1 && SD_SendCmd(CMD41, 1UL << 30) == 0)
                        break; /* ACMD41 with HCS bit */
                } while (timer1 > HAL_GetTick());

                if ((timer1 > HAL_GetTick()) && SD_SendCmd(CMD58, 0) == 0)
                {
                    /* Check CCS bit */
                    SPI_RxMultiByte(ocr, 4);
                    type = (ocr[0] & 0x40) ? 6 : 2;
                }
            }
        }
        else
        {
            /* SDC Ver1 or MMC */
            type = (SD_SendCmd(CMD55, 0) <= 1 && SD_SendCmd(CMD41, 0) <= 1) ? 2 : 1; /* SDC : MMC */

            do {
                if (type == 2)
                {
                    if (SD_SendCmd(CMD55, 0) <= 1 && SD_SendCmd(CMD41, 0) == 0)
                        break; /* ACMD41 */
                }
                else
                {
                    if (SD_SendCmd(CMD1, 0) == 0)
                        break; /* CMD1 */
                }
            } while (timer1 > HAL_GetTick());

            if (!(timer1 > HAL_GetTick()) || SD_SendCmd(CMD16, 512) != 0)
            {
                /*Select block length */
                type = 0;
            }
        }
    }

    CardType = type;

    DESELECT();

    SPI_RxByte(); /*Idle state transition(Release DO) */

    if (type)
    {
        /* Clear STA_NOINIT */
        Stat &= ~STA_NOINIT;
    }
    else
    {
        /* Initialization failed */
        SD_PowerOff();
    }

    return Stat;
}

/*Check disk status*/
DSTATUS SD_disk_status(BYTE drv)
{
    if (drv)
        return STA_NOINIT;

    return Stat;
}

/*Reading Sectors */
DRESULT SD_disk_read(BYTE pdrv, BYTE* buff, DWORD sector, UINT count)
{
    if (pdrv || !count)
        return RES_PARERR;

    if (Stat & STA_NOINIT)
        return RES_NOTRDY;

    if (!(CardType & 4))
        sector *= 512;      /* Change designated sector to Byte addressing unit*/

    SELECT();

    if (count == 1)
    {
        /* Single block reading */
        if ((SD_SendCmd(CMD17, sector) == 0) && SD_RxDataBlock(buff, 512))
            count = 0;
    }
    else
    {
        /* Multiblock Read */
        if (SD_SendCmd(CMD18, sector) == 0)
        {
            do {
                if (!SD_RxDataBlock(buff, 512))
                    break;

                buff += 512;
            } while (--count);

            /*STOP_TRANSMISSION, after reading all blocks, */
            SD_SendCmd(CMD12, 0);
        }
    }

    DESELECT();
    SPI_RxByte(); /* Idle state (Release DO) */

    return count ? RES_ERROR : RES_OK;
}

/* Sector writing*/
#if _READONLY == 0
DRESULT SD_disk_write(BYTE pdrv, const BYTE* buff, DWORD sector, UINT count)
{
    if (pdrv || !count)
        return RES_PARERR;

    if (Stat & STA_NOINIT)
        return RES_NOTRDY;

    if (Stat & STA_PROTECT)
        return RES_WRPRT;

    if (!(CardType & 4))
        sector *= 512; /*Change designated sector to Byte addressing unit */

    SELECT();

    if (count == 1)
    {
        /* Single block writing */
        if ((SD_SendCmd(CMD24, sector) == 0) && SD_TxDataBlock(buff, 0xFE))
            count = 0;
    }
    else
    {
        /* Multiblock Writing */
        if (CardType & 2)
        {
            SD_SendCmd(CMD55, 0);
            SD_SendCmd(CMD23, count); /* ACMD23 */
        }

        if (SD_SendCmd(CMD25, sector) == 0)
        {
            do {
                if(!SD_TxDataBlock(buff, 0xFC))
                    break;

                buff += 512;
            } while (--count);

            if(!SD_TxDataBlock(0, 0xFD))
            {
                count = 1;
            }
        }
    }

    DESELECT();
    SPI_RxByte();

    return count ? RES_ERROR : RES_OK;
}
#endif /* _READONLY */

/* Other functions */
DRESULT SD_disk_ioctl(BYTE drv, BYTE ctrl, void *buff)
{
    DRESULT res;
    BYTE n, csd[16], *ptr = buff;
    WORD csize;

    if (drv)
        return RES_PARERR;

    res = RES_ERROR;

    if (ctrl == CTRL_POWER)
    {
        switch (*ptr)
        {
            case 0:
                if (SD_CheckPower())
                    SD_PowerOff();          /* Power Off */
                res = RES_OK;
                break;
            case 1:
                SD_PowerOn();             /* Power On */
                res = RES_OK;
                break;
            case 2:
                *(ptr + 1) = (BYTE) SD_CheckPower();
                res = RES_OK;             /* Power Check */
                break;
            default:
                res = RES_PARERR;
        }
    }
    else
    {
        if (Stat & STA_NOINIT)
            return RES_NOTRDY;

        SELECT();

        switch (ctrl)
        {
            case GET_SECTOR_COUNT:
                /* 	Number of Sectors in SD Card (DWORD) */
                if ((SD_SendCmd(CMD9, 0) == 0) && SD_RxDataBlock(csd, 16))
                {
                    if ((csd[0] >> 6) == 1)
                    {
                        /* SDC ver 2.00 */
                        csize = csd[9] + ((WORD) csd[8] << 8) + 1;
                        *(DWORD*) buff = (DWORD) csize << 10;
                    }
                    else
                    {
                        /* MMC or SDC ver 1.XX */
                        n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
                        csize = (csd[8] >> 6) + ((WORD) csd[7] << 2) + ((WORD) (csd[6] & 3) << 10) + 1;
                        *(DWORD*) buff = (DWORD) csize << (n - 9);
                    }

                    res = RES_OK;
                }
                break;

            case GET_SECTOR_SIZE:
                /*Unit size of the sector (WORD) */
                *(WORD*) buff = 512;
                res = RES_OK;
                break;

            case CTRL_SYNC:
                /* Write synchronization */
                if (SD_ReadyWait() == 0xFF)
                    res = RES_OK;
                break;

            case MMC_GET_CSD:
                /* Receiving CSD information(16 bytes) */
                if (SD_SendCmd(CMD9, 0) == 0 && SD_RxDataBlock(ptr, 16))
                    res = RES_OK;
                break;

            case MMC_GET_CID:
                /* Receiving CID information(16 bytes) */
                if (SD_SendCmd(CMD10, 0) == 0 && SD_RxDataBlock(ptr, 16))
                    res = RES_OK;
                break;

            case MMC_GET_OCR:
                /* Receive OCR information(4 bytes) */
                if (SD_SendCmd(CMD58, 0) == 0)
                {
                    SPI_RxMultiByte(ptr, 4);

                    res = RES_OK;
                }
                break;
            default:
                res = RES_PARERR;
        }

        DESELECT();
        SPI_RxByte();
    }

    return res;
}
