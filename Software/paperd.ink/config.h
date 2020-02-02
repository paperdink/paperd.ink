
#ifndef CONFIG_H
#define CONFIG_H

#include <Adafruit_GFX.h>
#include <gfxfont.h>
#include "Gobold_Thin25pt7b.h"
#include "Gobold_Thin9pt7b.h"

// I2C pins
#define SDA 16
#define SCL 17

// SPI pins

// SD card pins
#define SD_CS 21

// E-paper pins
#define EPD_CS 22
#define EPD_DC 26
#define EPD_RST_DUM -1 // This is actually a dummy pin in this code, actually reset is controlled by the PCF8574
#define EPD_BUSY 35

// RGB pins
#define GREEN_LED_PIN 0
#define BLUE_LED_PIN 2
#define RED_LED_PIN 5

// LED Fade config
#define LEDC_CHANNEL_R     0
#define LEDC_CHANNEL_G     1
#define LEDC_CHANNEL_B     2
// use 13 bit precission for LEDC timer
#define LEDC_TIMER_13_BIT  13
// use 5000 Hz as a LEDC base frequency
#define LEDC_BASE_FREQ     5000
//extern int fade_amount;

// PCF8574 pins
#define EPD_EN P0
#define BATT_EN P1
#define SD_EN P2
#define EPD_RES P3
#define SD_CD P4 // input
#define EXT_GPIO1 P5
#define EXT_GPIO2 P6
#define EXT_GPIO3 P7
#define PCF_I2C_ADDR 0x20

// LiPo Charger
#define CHARGING_PIN 36
// Battry Voltage
#define BATTERY_VOLTAGE 34

// Fonts
#define PRIMARY_FONT &Gobold_Thin25pt7b
#define SECONDARY_FONT &Gobold_Thin9pt7b
#define SMALL_FONT &Gobold_Thin9pt7b

// Time ESP32 will go to sleep (in seconds)
#define TIME_TO_SLEEP  55

// todo list definitions
// memory allocated for getting json output from todoist
#define MAX_TODO_ITEMS 3
#define tasks_size MAX_TODO_ITEMS*500 // about 500 bytes per task
extern RTC_DATA_ATTR char todo_items[MAX_TODO_ITEMS][30];
extern uint16_t resp_pointer;
// memory to get the http response
extern char http_response[tasks_size]; // RGTODO: Make it dynamically allocated?
extern bool request_finished;

// weather definitions
#define weather_size 1000
extern RTC_DATA_ATTR char weather_icon[15];

// To keep track of number of times device booted
extern RTC_DATA_ATTR long long bootCount;

#define TZ_ENV "UTC-05:30" //See: https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html  

extern const char* ssid;
extern const char* password;
extern String todoist_token_base;
extern String openweathermap_link_base;
extern String time_zone;
#endif /* CONFIG_H */
