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
int fade_amounrt = 5;

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

#define TZ_ENV "UTC-05:30" //See: https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html  
#define TIME_TO_SLEEP  55        //Time ESP32 will go to sleep (in seconds)

const char* ssid = "*************";
const char* password =  "*************";
const char* todoist_token = "Bearer *************";
const char* openweathermap_link = "http://api.openweathermap.org/data/2.5/*************";
