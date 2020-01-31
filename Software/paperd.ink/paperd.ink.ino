/*
 * ------------------------------------------------------------
 * "THE BEERWARE LICENSE" (Revision 1): 10 Dec 2019
 * Rohit Gujarathi wrote this code. As long as you retain this 
 * notice, you can do whatever you want with this stuff. If we
 * meet someday, and you think this stuff is worth it, you can
 * buy me a beer in return.
 * ------------------------------------------------------------
 */
 
#include "FS.h"
#include "SPIFFS.h"
#include "SD.h"
#include "SPI.h"
#include <sys/time.h>
#include "WiFi.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "driver/adc.h"
#include <esp_wifi.h>
#include <esp_bt.h>
#include <DNSServer.h>
#include <WebServer.h>
#include "WiFiManager.h"          //https://github.com/zhouhan0126/WIFIMANAGER-ESP32

// #########  Configuration ##########
#include "config.h"
// ###################################

#include "PCF8574.h"
#include "GUI.h"
#include "date_time.h"
#include "led_fade.h"

#include <GxEPD.h>
#include <GxGDEW042T2/GxGDEW042T2.h>      // 4.2" b/w
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

// E-paper
GxIO_Class io(SPI, /*CS=5*/ EPD_CS, /*DC=*/ EPD_DC, /*RST=*/ EPD_RST_DUM);
GxEPD_Class display(io, /*RST=*/ EPD_RST_DUM, /*BUSY=*/ EPD_BUSY);

// PCF8574 GPIO extender
PCF8574 pcf8574(PCF_I2C_ADDR, SDA, SCL);

uint8_t wifi_connected = 1;
RTC_DATA_ATTR long long bootCount = 0; // keep track of boots

void setup(void)
{
  if (bootCount == 0) { // RGTODO: Add wakeup reason check also?
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(BLUE_LED_PIN, OUTPUT);
    pinMode(RED_LED_PIN, OUTPUT);
    digitalWrite(BLUE_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, HIGH);
    digitalWrite(RED_LED_PIN, HIGH);
  }
  
  Serial.begin(115200);
  Serial.println();
  Serial.println("paperd.ink");
  
  pcf8574.pinMode(EPD_EN, OUTPUT);
  pcf8574.pinMode(EPD_RES, OUTPUT);
  pcf8574.pinMode(SD_EN, OUTPUT);
  pcf8574.pinMode(BATT_EN, OUTPUT);
  pcf8574.begin();

  Serial.println("Turning things on");
  pcf8574.digitalWrite(EPD_EN, LOW);
  
  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS))
  {
    Serial.println("failed!");
  }else{
    Serial.println("OK!");
  }
  
  Serial.print("Initializing SPIFFS...");
  if(!SPIFFS.begin(true)){
    Serial.println("failed!");
    //return;
  }else{
    Serial.println("OK!");
  }
  delay(100);

  // Power up EPD
  pcf8574.digitalWrite(EPD_RES, LOW);
  delay(50);
  pcf8574.digitalWrite(EPD_RES, HIGH);
  delay(50);
  display.init(115200); // enable diagnostic output on Serial

  // clear the display
  display.fillScreen(GxEPD_WHITE);
  display.setRotation(0);
    
  if((now.hour % 6 == 0 && now.min == 0) || bootCount == 0){
    // All WiFi related activity once every 6 hours
    if(Start_WiFi(ssid,password)<0){
      Serial.printf("Can't connect to %s.",ssid);
    }else{
      // Fetch tasks
      request_finished = false;
      xTaskCreate(todo_task, "todo_task", (1024 * 32), NULL, 5, NULL);
      while(!request_finished){
        delay(500);
      }
      // Sync time
      //set_time(); //set time to a predefined value
      configTime(0, 0, "pool.ntp.org");
    }
  }

  // Battery status calculation
  adc_power_on();
  uint8_t not_charging = digitalRead(CHARGING_PIN);
  pcf8574.digitalWrite(BATT_EN, LOW);
  delay(10);
  uint16_t batt_adc = analogRead(BATTERY_VOLTAGE);
  adc_power_off();
  pcf8574.digitalWrite(BATT_EN, HIGH);
  //float batt_voltage = (float)((batt_adc/4095.0)*3.3);
  float batt_voltage = (float)((batt_adc/3970.0)*4.2);
  //float batt_voltage = 4.1;

  // Get the date details before any date/time related tasks
  get_date_dtls(); 
  
  display_battery(&display,batt_voltage,not_charging);
  display_weather(&display);
  display_background(&display);
  display_calender(&display);
  display_tasks(&display);
  display_time(&display); 
  
  display_update(&display);// display_update should be at the end as it controls the refresh type and displays the image
  
  Serial.println("Turning off everything");
  pcf8574.digitalWrite(SD_EN, HIGH);
  pcf8574.digitalWrite(BATT_EN, HIGH);
  pcf8574.digitalWrite(EPD_EN, HIGH);
  // Powerdown EPD
  //display.powerDown(); // Dont use this if you require partial updates
  pcf8574.digitalWrite(EPD_RES, LOW);
  delay(50);
  pcf8574.digitalWrite(EPD_RES, HIGH);
  
  ++bootCount;

  // Prepare to go to sleep
  if((now.hour % 6 == 0 && now.min == 0) || bootCount == 0){
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    btStop();
  
    esp_wifi_stop();
    esp_bt_controller_disable();
  }
  
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.printf("Going to sleep %d time...",bootCount);
  // Go to sleep
  esp_deep_sleep_start();
}

void loop()
{

}

// WiFi API
int8_t Start_WiFi(const char* ssid, const char* password){
 int connAttempts = 0;
 Serial.println("\r\nConnecting to: "+String(ssid));
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED ) {
   delay(500);
   Serial.print(".");
   if(connAttempts > 40) return -1;
   connAttempts++;
 }
 Serial.println("");
 return 0;
}
