/*
 * system.c
 *
 *  Created on: 27.04.2022
 *      Author: Karol
 */

#include "system.h"
#include "fatfs.h"
#include "logger.h"
#include <string.h>
#include "main.h"
#include "rtc.h"
#include "sw_watchdog.h"

#include "wifi/wifi_esp.h"

#include "e-Paper/EPD_1in54b.h"
#include "Config/DEV_Config.h"
#include "GUI/GUI_Paint.h"
#include "e-Paper/ImageData.h"

#include "images.h"
#include "led.h"
#include "button.h"
#include "backup_registers.h"

#include <stdio.h>

typedef enum system_rst_src
{
    SYSTEM_PWR_RST = 0,
    SYSTEM_WAKEUP_RST,
    SYSTEM_ALARM_RST,
    SYSTEM_WATCHDOG_RST,
    SYSTEM_UNKNOWN_RST
} system_rst_src;

typedef struct SSystemConfig
{
    uint8_t configMode;
    uint8_t ledIndON;
    uint32_t wakeUpCounter;
    system_rst_src resetSrc;
    char hostName[33];
} SSystemConfig;


static FATFS fs;
static SSystemConfig systemConfig = {
        .configMode = 0,
        .ledIndON = 1,
        .wakeUpCounter = 0,
        .resetSrc = SYSTEM_PWR_RST,
        .hostName = ""
};

static uint32_t startTimestamp = 0;


//private functions
static void shutdownSystem(void);
static void cardMountFailed(FRESULT status);


void system_init(void)
{
    FIL file;
    uint32_t startRTCTime;

    startTimestamp = HAL_GetTick();
    startRTCTime = (RTC_getTime() & 0xFFFF);

    //wait for release wakeup button
    bool wkupButtonPressed = false;
    while(1)
    {
        if(!button_isWakeUpPressed())
        {
            system_sleep(100);
            if(!button_isWakeUpPressed())
            {
                break;
            }
        }
        else
        {
            wkupButtonPressed = true;
            system_sleep(100);
        }
    }

    if(sw_watchdog_isResetCause())
    {
        systemConfig.resetSrc = SYSTEM_WATCHDOG_RST;
        led_setColor(LED_YELLOW);
    }
    else if(__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET)
    {
        uint32_t lastAlarmTime = BCKUP_getLastAlarm();

        /* Clear standby flag */
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);


        if(wkupButtonPressed)
        {
            systemConfig.resetSrc = SYSTEM_WAKEUP_RST;
        }
        else if(lastAlarmTime + 1 == startRTCTime)
        {
            systemConfig.resetSrc = SYSTEM_ALARM_RST;
        }
        else
        {
            systemConfig.resetSrc = SYSTEM_UNKNOWN_RST;
        }

        systemConfig.wakeUpCounter = BCKUP_getWkupCnt();

        systemConfig.wakeUpCounter++;
    }

    BCKUP_setWkupCnt(systemConfig.wakeUpCounter);

    if(__HAL_PWR_GET_FLAG(PWR_CSR_EWUP) == RESET)
    {
        HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    }

    /* Start battery voltage measurement*/
    system_powerInit();

    /* Mount SD Card */
    FRESULT f_status = f_mount(&fs, "", 1);
    if(FR_OK != f_status)
    {
        cardMountFailed(f_status);
    }
    system_sleep(100);

    /* Logger init */
    Logger_init();
    Logger(LOG_VIP, "SN: %08X%08X%08X", HAL_GetUIDw2(), HAL_GetUIDw1(), HAL_GetUIDw0());
    Logger(LOG_VIP, "Version: %s", VERSION_STR);

    Logger(LOG_VIP, "Reset cause: %d", systemConfig.resetSrc);
    if(SYSTEM_UNKNOWN_RST == systemConfig.resetSrc)
    {
        Logger(LOG_WRN, "Unknown reset cause");
    }
    else if (SYSTEM_WATCHDOG_RST == systemConfig.resetSrc)
    {
        Logger(LOG_WRN, "Reset by watchdog!");
    }

#ifdef DEBUG
    Logger_setMinLevel(LOG_DBG);
#endif

    /* Factory reset (button) */
    if(SYSTEM_PWR_RST == systemConfig.resetSrc)
    {
        uint32_t counter = 0;
        while(button_isFactoryResetPressed())
        {
            led_setColor(LED_CYAN);
            system_sleep(100);
            counter++;
        }

        if((counter) > (5000/100))	//wait 5 seconds
        {
            Logger(LOG_VIP, "Factory reset...");
            system_restoreDefault();
            system_restart(1);
        }
    }

    sw_watchdog_enable(60000); //60 seconds

    /* Check if config & system directories exist */
    if(FR_NO_FILE == f_stat(DIR_PATH_SYS, NULL) || FR_NO_FILE == f_stat(DIR_PATH_CONF, NULL))
    {
        led_setColor(LED_CYAN);
        Logger(LOG_VIP, "Required directories not found! Restoring...");
        system_sleep(500);
        system_restoreDefault();
        system_restart(1);
    }

    //read calibrated RTC clock
    RTC_loadCalibratedClock();

    /* Read configs */
    if(FR_OK == f_stat(FILE_PATH_LED_IND_FLAG, NULL))
    {
        systemConfig.ledIndON = 1;
    }
    else
    {
        systemConfig.ledIndON = 0;
        led_setColor(LED_OFF);
    }

    if(FR_OK == f_open(&file, FILE_PATH_HOSTNAME, FA_READ))
    {
        if(NULL == f_gets(systemConfig.hostName, 33, &file))
        {
            systemConfig.hostName[0] = '\0';
        }
        f_close(&file);
    }

    if(button_isConfigModePressed())
    {
        systemConfig.configMode += 1;
    }

    if(FR_OK == f_stat(FILE_PATH_CONFIG_MODE_FLAG, NULL))
    {
        systemConfig.configMode += 2;
        f_unlink(FILE_PATH_CONFIG_MODE_FLAG);
    }

    if(systemConfig.configMode) led_setColor(LED_BLUE);

    Logger(LOG_INF, "Config mode %d", systemConfig.configMode);

    /* Display init */
    if(EPD_Init())
    {
        Logger(LOG_ERR, "e-Paper init failed");
    }

    if(systemConfig.configMode || systemConfig.wakeUpCounter % 5 == 0)
    {
        Logger(LOG_INF, "clear disp");
        EPD_Clear();
    }

    Logger(LOG_INF, "system init OK (%d)", systemConfig.wakeUpCounter);
}

void system_shutdown(void)
{
    shutdownSystem();

    /* Clear wakeup flag */
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

    HAL_PWR_EnterSTANDBYMode();
}

void system_restart(uint8_t configMode)
{
    Logger(LOG_INF, "Prepare for restart in %s mode", configMode ? "config" : "normal");
    if(configMode)
    {
        FIL file;
        if(FR_OK == f_open(&file, FILE_PATH_CONFIG_MODE_FLAG, FA_OPEN_ALWAYS))
        {
            f_close(&file);
        }
    }

    shutdownSystem();
    NVIC_SystemReset();
}

void system_setWakeUpTimer(uint32_t seconds)
{
    time_t timeAlarm = RTC_getTime();
    timeAlarm += seconds;
    Logger(LOG_INF, "Wake-up signal in %d seconds", seconds);

    BCKUP_setLastAlarm(timeAlarm);

    RTC_setAlarmTime(timeAlarm);
}

char* const system_getHostName(void)
{
    return systemConfig.hostName;
}

uint8_t system_isLedIndicatorOn(void)
{
    return systemConfig.ledIndON;
}

uint8_t system_isConfigModeOn(void)
{
    return systemConfig.configMode;
}

void system_sleep(uint32_t miliseconds)
{
    uint32_t tickstart = HAL_GetTick();
    uint32_t wait = miliseconds;

    /* Add a freq to guarantee minimum wait */
    if (wait < HAL_MAX_DELAY)
    {
        wait += (uint32_t)(uwTickFreq);
    }

    while ((HAL_GetTick() - tickstart) < wait)
    {
        HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
    }
}

uint8_t system_restoreDefault(void)
{
    FIL file;
    int8_t result = 0;

    Logger(LOG_WRN, "restore default settings");

    //create directories
    f_mkdir(DIR_PATH_SYS);
    f_mkdir(DIR_PATH_CONF);

    //WiFi config
    f_unlink(FILE_PATH_WIFI_CONFIG);

    //forecast settings
    f_unlink(FILE_PATH_FORECAST_CONFIG);

    //remove RTC calibration
    f_unlink(FILE_PATH_RTC_CALIBRATION);

    //led indicator
    if(0 == result)
    {
        if(FR_OK == f_open(&file, FILE_PATH_LED_IND_FLAG, FA_OPEN_ALWAYS))
        {
            f_close(&file);
        }
        else
        {
            result = 1;
        }
    }

    //host name
    if(0 == result)
    {
        if(FR_OK == f_open(&file, FILE_PATH_HOSTNAME, FA_OPEN_ALWAYS | FA_WRITE))
        {
            f_printf(&file, "WeatherStation_%04X", (uint16_t)HAL_GetUIDw0());
            f_close(&file);
        }
        else
        {
            result = 1;
        }
    }

    //access point config
    if(0 == result)
    {
        if(FR_OK == f_open(&file, FILE_PATH_AP_CONFIG, FA_OPEN_ALWAYS | FA_WRITE))
        {
            f_printf(&file, "{\"ssid\":\"WeatherStation_%04X\",\"enc\":3,\"channel\":8,\"pass\":\"littlefox123\"}", (uint16_t)HAL_GetUIDw0());
            f_close(&file);
        }
        else
        {
            result = 1;
        }
    }
    return result;
}

static void shutdownSystem(void)
{
    Logger(LOG_INF, "System shutdown");
    EPD_Sleep();
    WiFi_DisconnectStation(1000);
    WiFi_shutdown();

    Logger(LOG_INF, "Run time: %u ms", HAL_GetTick()-startTimestamp);
    Logger_shutdown();
    system_sleep(100);
    disk_ioctl(fs.drv, CTRL_POWER, 0);
    f_mount(NULL, "", 1);
}

static void cardMountFailed(FRESULT status)
{
    char errStr[20];
    led_setColor(LED_RED);
    Logger(LOG_VIP, "SD card err %d", status);
    EPD_Init();

    sprintf(errStr, "Err %d", status);
    show_error_image(ERR_IMG_MEMORY_CARD, "Memory Error", errStr);
    EPD_Sleep();
    /* Clear wakeup flag */
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    HAL_PWR_EnterSTANDBYMode();
}
