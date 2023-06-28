# Preparation for first start

## WiFi module

The device's WiFi module should be programed with AT command firmware. The firmware and all needed tools can be found in links below.

ESP32 AT module firmware:
https://docs.espressif.com/projects/esp-at/en/latest/esp32c3/AT_Binary_Lists/ESP32-C3_AT_binaries.html

ESP32 tools and downloading guide:
https://docs.espressif.com/projects/esp-at/en/latest/esp32c3/Get_Started/Downloading_guide.html

### Jumpers and connectors:
- JP1 - select ESP_Enable signal source: 1-2: MCU, 2-3: module always enabled
- J5 - Jumper on: download mode. Jumper off: normal mode
- J7 - ESP32 uart connector: 1 - RXD, 2 - TXD, 3 - GND 

Before flashing, the JP1 solder bridge should connect pins 2 and 3 to make sure the module is powered on when firmware downloading.
Jumper J5 must be connected before powering on to enter download mode.

## MCU

- J1 - Connect to reset STM32F1 MCU.
- J3 - MCU programming connector: 1 - SWDIO, 2 - SWCLK, 3 - GND

## SD Card preparation
The device uses an SD memory card to store static HTML files, graphics, configurations and logs. The first partition on the memory card should be formatted as FAT32 with a block size of 512B. Partition size can be up to 2GB.

Fresh formated memory card should contains directories:
- `img` - contains images (see [Generation of display images](../res/README.md))
- `html` - contains static html files (see [Local development server](../html/README.md))