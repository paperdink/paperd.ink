
#ifndef CONFIG_H
#define CONFIG_H

#include "Mont_HeavyDEMO25pt7b.h"
#include "Gobold_Thin9pt7b.h"
#include "Mont_ExtraLightDEMO8pt7b.h"

// DETAILS TO EDIT
#define SSID     "*****" // your network SSID (name of wifi network)
#define PASSWORD "*****" // your network password
#define TODOIST_TOKEN "Bearer *******" // your todoist API key
#define CITY "*****" // your city for weather
#define COUNTRY "*****" // your country for weather
#define OWM_ID "*****" // your open weather map APP ID
#define TIME_ZONE "-05:30" // your time zone

#define DEBUG Serial

// JSON buffer size for tasks
#define tasks_size 3000

// PIN ASSIGNMENT

// I2C pins
#define SDA 16
#define SCL 17

// SPI pins

// SD card pins
#define SD_CS 21
#define SD_EN 5

// E-paper pins
#define EPD_CS 22
#define EPD_DC 15
#define EPD_BUSY 34
#define EPD_EN 12
#define EPD_RES 13

// PCF8574 pins
#define PCF_INT 35
#define SD_CD P4 // input
#define EXT_GPIO1 P5
#define EXT_GPIO2 P6
#define EXT_GPIO3 P7
#define PCF_I2C_ADDR 0x20

// LiPo
#define CHARGING_PIN 36
#define BATT_EN 25
#define BATTERY_VOLTAGE 34

// Fonts
#define LARGE_FONT &Mont_HeavyDEMO25pt7b
#define MED_FONT &Gobold_Thin9pt7b
#define SMALL_FONT &Mont_ExtraLightDEMO8pt7b

#endif /* CONFIG_H */
