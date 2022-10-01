/*
 * system_info.h
 *
 *  Created on: 10.04.2022
 *      Author: Karol
 */

#ifndef SYSTEM_INFO_H_
#define SYSTEM_INFO_H_

#ifdef DEBUG
#define VERSION_STR __DATE__ "_dbg"
#else
#define VERSION_STR __DATE__
#endif


//----FILES----//

#define DIR_PATH_SYS "/sys"
#define DIR_PATH_CONF "/conf"

#define FILE_PATH_AP_CONFIG			DIR_PATH_CONF"/ap.conf"
#define FILE_PATH_WIFI_CONFIG		DIR_PATH_CONF"/wifi.conf"
#define FILE_PATH_FORECAST_CONFIG	DIR_PATH_CONF"/forecast.conf"

#define FILE_PATH_HOSTNAME			DIR_PATH_SYS"/hostname"
#define FILE_PATH_LED_IND_FLAG		DIR_PATH_SYS"/led_ind"
#define FILE_PATH_CONFIG_MODE_FLAG  DIR_PATH_SYS"/conf_mode"

#endif /* SYSTEM_INFO_H_ */
