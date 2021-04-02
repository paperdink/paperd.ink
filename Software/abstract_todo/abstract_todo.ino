

#include <WiFiClientSecure.h>
#include <SD.h>
#include <SPI.h>
#include <FS.h>
#include <SPIFFS.h>
#include <GxEPD.h>
#include <GxGDEW042T2/GxGDEW042T2.h>      // 4.2" b/w
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include <sys/time.h>

#include "config.h"
#include "PCF8574.h"
#include "gui.h"
#include "date_time.h"

// E-paper
GxIO_Class io(SPI, /*CS=5*/ EPD_CS, /*DC=*/ EPD_DC, /*RST=*/ EPD_RES);
GxEPD_Class display(io, /*RST=*/ EPD_RES, /*BUSY=*/ EPD_BUSY);

int8_t connect_wifi();

void setup() {
  DEBUG.begin(115200);
  DEBUG.println();
  DEBUG.println("paperd.ink");

  pinMode(EPD_EN, OUTPUT);
  pinMode(EPD_RES, OUTPUT);
  pinMode(SD_EN, OUTPUT);
  pinMode(BATT_EN, OUTPUT);
  pinMode(PCF_INT, INPUT); // Required to lower power consumption

  // Power up EPD
  digitalWrite(EPD_EN, LOW);
  digitalWrite(EPD_RES, LOW);
  delay(50);
  digitalWrite(EPD_RES, HIGH);
  delay(50);
  //display.init(115200); // enable diagnostic output on DEBUG
  display.init();
 
  DEBUG.print("Initializing SPIFFS...");
  if(!SPIFFS.begin(true)){
    DEBUG.println("failed!");
    //return;
  }else{
    DEBUG.println("OK!");
  }
  delay(100);

  // TODO: if battery is low then dont connect wifi else brownout
  if(connect_wifi() == 0){
    configTime(0, 0, "pool.ntp.org");
    if(get_date_dtls(TIME_ZONE) < 0){
      configTime(0, 0, "pool.ntp.org");
    }
  }else{
    get_date_dtls(TIME_ZONE);
  }
  
  drawBitmapFrom_SD_ToBuffer(&display, SPIFFS, "5.bmp", 0, 0, 0);
  
  display_tasks(&display);
  diplay_date(&display);
  display_weather(&display);
  
  display.update();

  Serial.println("Turning off everything");
  digitalWrite(SD_EN, HIGH);
  digitalWrite(BATT_EN, HIGH);
  digitalWrite(EPD_EN, HIGH);
  // Powerdown EPD
  //display.powerDown(); // Dont use this if you require partial updates
  digitalWrite(EPD_RES, LOW);
  delay(50);
  digitalWrite(EPD_RES, HIGH);

  // Sleep till 12am
  uint64_t sleep_time = 86400-((now.mil_hour*3600)+(now.min*60)+(now.sec));
  esp_sleep_enable_timer_wakeup(sleep_time*uS_TO_S_FACTOR);
  Serial.printf("Going to sleep for %lld seconds...", sleep_time);
  // Go to sleep
  esp_deep_sleep_start();
}

void loop() {
  // put your main code here, to run repeatedly:

}

int8_t connect_wifi(){
 uint8_t connAttempts = 0;
 WiFi.begin(SSID, PASSWORD);
 while (WiFi.status() != WL_CONNECTED ) {
   delay(500);
   Serial.print(".");
   if(connAttempts > 40){
    return -1;
   }
   connAttempts++;
 }
 Serial.println("Connected");
 return 0;
}
