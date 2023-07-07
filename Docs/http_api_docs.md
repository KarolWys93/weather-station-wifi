# API Docs

## HTTP GET

### GET /api/status
Returns the current state of the device.

Response code: 200

Example of response:

    {
	  "wifi_ssid": "Testowa Sieć",
	  "wifi_stat": 1,
	  "bat": 43,
	  "ip_addr": "192.168.0.129",
	  "charging": 2,
	  "ver": "v1.3.1",
	  "sta_mac": "00:11:22:33:44:55",
	  "ap_mac": "AA:BB:CC:DD:EE:FF",
	  "ver_wifi": "2.1"
    }
Fields in response:

 - **wifi_ssid** - network name
 - **wifi_stat** - connection state: 1 - connected, 0 - disconnected
 - **bat** - battery level in %
 - **ip_addr** - device IP address
 - **charging** - charging status: 0 - no charging, 1 - charging, 2 - power supply on.
 - **ver** - software version
 - **sta_mac** - device MAC address (WiFi client)
 - **ap_mac** - MAC address of the access point (configuration mode)
 - **ver_wifi** - firmware version of the Wi-Fi module

### GET /api/ap_conf
Returns the current network settings of the access point (config mode).

Response code: 200

Example of response:

    {
	  "ap_ssid":"ConfigMode WiFi",
	  "ap_enc":"3",
	  "ap_chl":"8"
    }
Fields in response:
 - **ap_ssid** - network name
 - **ap_enc** - Access Point encryption: 0 - open, 2 - WPA PSK, 3 - WPA2 PSK, 4 - WPA WPA2 PSK
 - **ap_chl** - Wi-Fi channel (1-13)

### GET /api/ap_list
Returns Wi-Fi networks in range.

Response code: 200

Example of response:

    [
      {
        "ssid":"Example WiFi",
        "rssi":-40,
        "enc":4
      },
      {...},...
    ]
Fields in response:
 - **ssid** - network name
 - **enc** - encryption: 0 - open, 2 - WPA PSK, 3 - WPA2 PSK, 4 - WPA WPA2 PSK
 - **rssi** - Wi-Fi signal strength (dBm)

### GET /api/wifi_conf
Returns the current network settings.

Response code: 200

Example of response:

    {
	  "wifi_ssid": "Example WiFi",
	  "dhcp": 1,
	  "wifi_ip": "192.168.0.150",
	  "wifi_mask": "255.255.255.0",
	  "wifi_gw": "192.168.0.1"
    }
Fields in response:
 - **wifi_ssid** - network name
 - **dhcp** - dhcp status: 0 - disable, 1 - enable. Optional if DHCP is disabled.
 - **wifi_ip** - device IP address. Optional if DHCP is enabled.
 - **wifi_mask** - IP mask. Optional if DHCP is enabled.
 - **wifi_gw** - gateway IP address. Optional if DHCP is enabled.

### GET /api/fcast_conf
Returns the current forecast settings.

Response code: 200

Example of response where weather forecast location based on zip code:

    {
	  "api_key":"1234567890asdfghjklpqwertyuiop32",
	  "refresh":2,
	  "country":"PL",
	  "zip_code":"12-345"
    }
and for weather forecast location based on coordinates:

    {
	  "api_key":"1234567890asdfghjklpqwertyuiop32",
	  "refresh":2,
	  "lon": "89.9999999",
	  "lat": "89.9999999"
    }
Fields in responses:
 - **api_key** - geolocation service api key
 - **refresh** - how often the weather forecast is refreshed in hours
 - **country** - country code (ISO 3166-1 alpha-2 code)
 - **zip_code** - zip_code
 - **lon** - longitude
 - **lat** - latitude

### GET /api/system_conf
Returns the current system settings.

Response code: 200

Example of response:

    {
	  "device_name": "Potato1234",
	  "led_ind": 1
    }
Fields in response:
 - **device_name** - name of the device
 - **led_ind** - LED status indicator: 0 - off, 1 - on

### GET /api/restart
Restarts the device in forecast mode.

Response code: 204

### GET /api/restart_conf
Restarts the device in configuration mode.

Response code: 204

### GET /api/restore_config
Restores factory settings.

Response code: 204


## HTTP POST

### POST /api/ap_conf
Set access point configuration (for config mode).

Example request body:

    {
		"ap_ssid": "example_wifi",
		"ap_pass": "secret_password",
		"ap_enc": 4,
		"ap_chl": 8
	}

Request fields:
 - **ap_ssid** - Access Point WiFi name (max 31 chars)
 - **ap_enc** - Access Point encryption: 0 - open, 2 - WPA PSK, 3 - WPA2 PSK, 4 - WPA WPA2 PSK
 - **ap_pass** - Access Point password (min 8, max 62 chars). Not needed if ***ap_enc = 0***
 - **ap_chl** - channel of the access point (1-13)

Response code: 204

### POST /api/wifi_conf
Set WiFi configuration.

Example request body:

    {
		"ssid": "example_wifi",
		"pass": "secret_password",
		"dhcp": 0,
	    "ip": "192.168.0.150",
	    "mask": "255.255.255.0",
	    "gw": "192.168.0.1"
	}

Request fields:
 - **ssid** - WiFi name (max 31 chars)
 - **pass** - Access Point password. If there is no encryption, you can skip it.
 - **dhcp** - dhcp: 0 - disable, 1 - enable. If the field is omitted, DHCP is disabled.
 - **ip** - Device IP address. Required if DHCP is disabled.
 - **mask** - IP mask. Required if DHCP is disabled.
 - **gw** - Gateway IP address. Required if DHCP is disabled.

Response code: 200

Example of response:

    {
	  "code": 0
    }
    
Fields in response:
**code**: 
- 0: connected to the new WiFi station.
- 1: connection timeout.
- 2: wrong password.
- 3: cannot find the target AP.
- 4: connection failed.
- 5: Cannot disable DHCP or set IP addresses.

### POST /api/fcast_conf
Set forecast configuration. Forecast will use geolocation service if zip code and country code are present. Otherwise, the location is based on coordinates.

Example request body:

    {-
      "refresh": 2,
      "country": "PL",
      "zip_code": "12-345",
      "api_key": "1234567890asdfghjklpqwertyuiop32",
      "lon": 8.9999,
      "lat": 8.9999
    }
Request fields:
 - **refresh** - how often the weather forecast will be refreshed in hours. Must be greater than 0.
 - **country** (optional)- country code (ISO 3166-1 alpha-2 code)
 - **zip_code** (optional) - zip_code
 - **api_key** - geolocation service API key. Required if country or zip_code fields are present.
 - **lon** - longitude. Not nedded if country or zip_code fields are present.
 - **lat** - latitude. Not nedded if country or zip_code fields are present.

Response code: 204

### POST /api/device_name
Set device name.

Example request body:

    {
      "device_name": "example_device_name"
    }
Request fields:
 - **device_name** - Device name. The maximum length is 32 characters.

Response code: 204

### POST /api/led_ind
Set device name.

Example request body:

    {
      "led_ind": 1
    }
Request fields:
 - **led_ind** - 0: disable LED indicator, 1: enable LED indicator

Response code: 204


## Debug API

### GET /api/logs?size={size}
Returns device logs.

Call parameters:
 - **size** (optional) - Maximum log size in kB. The maximum value is 512, the default is 20.

Response code: 200

### GET /api/voltage
Returns power supply information.

Response code: 200

Example of response:

    {
      "voltage":"4021",
      "bat_lvl":99,
      "bat_chr":%u
    }

Fields in response:

 - **voltage** - battery voltage in mV
 - **bat_lvl** - battery level in %
 - **bat_chr** - charging status: 0 - no charging, 1 - charging, 2 - power supply on.

### GET /api/led_test
Checks the LED indicator. The LED indicator flashes through all combinations of 8 colors.

Response code: 204

### GET /api/img_test?img={int}&text={text}&sub_text={text}

To check the display, it shows the selected image with text on the device.
Returns device logs.
Call parameters:
 - **text** (optional) - primary text, no longer than 40 bytes. Default empty.
 - **sub_text** (optional) - secondary text, no longer than 50 bytes. Default empty.
 - **img** (optional) - Image number. Default value is 2.

Images:

 0. memory card error image
 1. blank forecast background (text and subtext are not used)
 2. config mode image (text and subtext are not used)
 3. low battery image (text and subtext are not used)
 4. WiFi error image
 5. General error image

Response code: 204 on success or 400 on invalid img number

### GET /api/fcast_tmp
Returns the last forecast file

Response code: 200 on success or 404 if the file does not exist.
