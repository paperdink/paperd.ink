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

// WiFi Manager for initial configuration
WiFiManager wifiManager;
WiFiManagerParameter city_param("city", "City", NULL, 30, "");
WiFiManagerParameter country_param("country", "Country", NULL, 30, "");
WiFiManagerParameter todoist_token_param("todoist_token", "Todoist token", NULL, 41, "");
WiFiManagerParameter openweather_appkey_param("openweather_appkey", "OpenWeather appkey", NULL, 33, "");
WiFiManagerParameter time_zone_param("time_zone", "Timezone", NULL, 7, "");

RTC_DATA_ATTR char city_string[30];
RTC_DATA_ATTR char country_string[30];
RTC_DATA_ATTR char todoist_token_string[42];
RTC_DATA_ATTR char openweather_appkey_string[34];
RTC_DATA_ATTR char time_zone_string[7];

String todoist_token_base = "Bearer ";
String openweathermap_link_base = "http://api.openweathermap.org/data/2.5/weather?q={p},{c}&appid={i}";

RTC_DATA_ATTR uint8_t wifi_connected = 0; // keep track if wifi was connected and according update the symbol
RTC_DATA_ATTR long long bootCount = 0; // keep track of boots
esp_sleep_wakeup_cause_t wakeup_reason;

void setup(void)
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("paperd.ink");

  pcf8574.pinMode(EPD_EN, OUTPUT);
  pcf8574.pinMode(EPD_RES, OUTPUT);
  pcf8574.pinMode(SD_EN, OUTPUT);
  pcf8574.pinMode(BATT_EN, OUTPUT);
  pcf8574.begin();

  // Power up EPD
  pcf8574.digitalWrite(EPD_EN, LOW);
  pcf8574.digitalWrite(EPD_RES, LOW);
  delay(50);
  pcf8574.digitalWrite(EPD_RES, HIGH);
  delay(50);
  display.init(115200); // enable diagnostic output on Serial

  if (bootCount == 0) {
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(BLUE_LED_PIN, OUTPUT);
    pinMode(RED_LED_PIN, OUTPUT);
    digitalWrite(BLUE_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, HIGH);
    digitalWrite(RED_LED_PIN, HIGH);
    if(wakeup_reason != ESP_SLEEP_WAKEUP_TIMER && wakeup_reason != ESP_SLEEP_WAKEUP_TOUCHPAD){
      // first boot into config mode and delete any old credentials
      WiFi.mode(WIFI_STA);
      WiFi.disconnect(true,true);
      delay(1000);
      
      if(config_paperd_ink(&display) < 0){
        // Error occured while configuring so reset paperd_ink
        WiFi.mode(WIFI_STA);
        WiFi.disconnect(true,true);
        delay(1000);
        esp_deep_sleep_start();
      }
    }
  }
  
/*
  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS))
  {
    Serial.println("failed!");
  }else{
    Serial.println("OK!");
  }
*/
  
  Serial.print("Initializing SPIFFS...");
  if(!SPIFFS.begin(true)){
    Serial.println("failed!");
    //return;
  }else{
    Serial.println("OK!");
  }
  delay(100);

  // clear the display
  display.fillScreen(GxEPD_WHITE);
  display.setRotation(0);
    
  if((now.hour % 6 == 0 && now.min == 0) || bootCount == 0){
    // All WiFi related activity once every 6 hours
    if(wifi_manager_connect(&wifiManager,1) < 0){
      Serial.printf("Can't connect to WiFi");
      wifi_connected = 0;
    }else{
      wifi_connected = 1;
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
  get_date_dtls(time_zone_string);
  
  display_battery(&display,batt_voltage,not_charging);
  display_wifi(&display,wifi_connected);
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
  
  // Prepare to go to sleep
  if((now.hour % 6 == 0 && now.min == 0) || bootCount == 0){
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    btStop();
    esp_wifi_stop();
    esp_bt_controller_disable();
  }

  //if(bootCount == 0){
    get_date_dtls(time_zone_string);
    esp_sleep_enable_timer_wakeup((60-now.sec) * uS_TO_S_FACTOR);
  //}else{
    //esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  //}
  
  bootCount = 1;
  Serial.println(city_string);
  Serial.println(country_string);
  Serial.println(todoist_token_string);
  Serial.println(openweather_appkey_string);
  Serial.println(time_zone_string);

  Serial.printf("Going to sleep...");
  // Go to sleep
  esp_deep_sleep_start();
}

void loop()
{

}

int8_t config_paperd_ink(GxEPD_Class* display){
  display_config_gui(display);
  
  if(wifi_manager_connect(&wifiManager,0) < 0){
    return -1;
  }

  return 0;
}

// WiFi Manager APIs
void configModeCallback(WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  
  // AP has started
  stop_led_fade();
  start_led_fade(240); // 240 is reddish hue
}

void sta_connected(){
  // Device is connected to proper SSID
  Serial.println("Connected to new AP.");
  stop_led_fade();
// delay(3000); // delay required for smooth fade
//  start_led_fade(80);
  digitalWrite(GREEN_LED_PIN,LOW); 
  delay(500);
  digitalWrite(GREEN_LED_PIN,HIGH);
}

void user_connected(){
  // User has connected to the AP we are broadcasting
  Serial.println("User connected to our AP.");
  stop_led_fade();
  digitalWrite(BLUE_LED_PIN,LOW);

// delay(2000); // delay required for smooth fade
// start_led_fade(180);
}

int8_t wifi_manager_connect(WiFiManager* wifiManager, uint8_t STA_first){
  
  //Sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep in seconds
  wifiManager->setTimeout(180);
  wifiManager->setConnectTimeout(30);
  
  wifiManager->setAPCallback(configModeCallback);
  wifiManager->setSaveConfigCallback(sta_connected);
  wifiManager->setUserConnectedCallback(user_connected);
  
  wifiManager->addParameter(&city_param);
  wifiManager->addParameter(&country_param);
  wifiManager->addParameter(&todoist_token_param);  
  wifiManager->addParameter(&openweather_appkey_param); 
  wifiManager->addParameter(&time_zone_param); 
  
  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with Paperd.Ink_<chip_id>
  //and goes into a blocking loop awaiting configuration
  if(!wifiManager->autoConnect(STA_first)) {
    Serial.println("Failed to connect and hit timeout");
    stop_led_fade();
    return -1;
  }
  Serial.println("Succesful connection to WiFi");

  // copy values to RTC memory
  // RGTODO: Save to spiffs in json
  strncpy(city_string,city_param.getValue(),30);
  strncpy(country_string,country_param.getValue(),30);
  strncpy(todoist_token_string,todoist_token_param.getValue(),42);
  strncpy(openweather_appkey_string,openweather_appkey_param.getValue(),34);
  strncpy(time_zone_string,time_zone_param.getValue(),7);
  
  return 0;
}
