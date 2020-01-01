/*
 * ------------------------------------------------------------
 * "THE BEERWARE LICENSE" (Revision 1): 10 Dec 2019
 * Rohit Gujarathi wrote this code. As long as you retain this 
 * notice, you can do whatever you want with this stuff. If we
 * meet someday, and you think this stuff is worth it, you can
 * buy me a beer in return.
 * ------------------------------------------------------------
 */
 
// has support for FAT32 support with long filenames
#include "FS.h"
#include "SPIFFS.h"
#include "SD.h"
#include "SPI.h"
#include <sys/time.h>
#include "WiFi.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>

extern "C" {
#include "sh2lib.h"
}

#define seekSet seek

// include library, include base class, make path known
#include <GxEPD.h>
#include <GxGDEW042T2/GxGDEW042T2.h>      // 4.2" b/w

#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>


#include "Gobold_Thin25pt7b.h"
#include "Gobold_Thin9pt7b.h"
#include <Fonts/FreeMonoBold9pt7b.h>
#include GxEPD_BitmapExamples

#define PRIMARY_FONT &Gobold_Thin25pt7b
#define SECONDARY_FONT &Gobold_Thin9pt7b

// #########  Configuration ##########
#include "config.h"
/*
#define SD_CS 21
#define EPD_CS 22
#define GREEN 0
#define BLUE 2
#define RED 5
#define TZ_ENV "UTC-05:30" //See: https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html  
#define TIME_TO_SLEEP  55        //Time ESP32 will go to sleep (in seconds)
const char* ssid = "*************";
const char* password =  "*************";
const char* todoist_token = "Bearer *************";
const char* openweathermap_link = "http://api.openweathermap.org/data/2.5/*************";
*/
// ###################################

// E-paper
GxIO_Class io(SPI, /*CS=5*/ EPD_CS, /*DC=*/ 26, /*RST=*/ 17);
GxEPD_Class display(io, /*RST=*/ 17, /*BUSY=*/ 35);

// todo list definitions
bool request_finished = false;
#define tasks_size 1500 // about 500 bytes per task
StaticJsonDocument<tasks_size> tasks;
char http_response[tasks_size];
#define weather_size 1000
StaticJsonDocument<weather_size> weather_json;
uint16_t resp_pointer = 0;
uint8_t todo_items_num = 3;

// function declaration with default parameter
void drawBitmapFromSD(const char *filename, int16_t x, int16_t y, bool with_color = true);

uint8_t wifi_connected = 1;

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR char weather_icon[15] = "Error.bmp";
RTC_DATA_ATTR char todo_items[3][30];

// Time definitions
struct time_struct {
  char  wday[4];
  char  month[4];
  uint8_t month_num;
  uint8_t mday;
  uint8_t mil_hour;
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
  uint8_t day_offset; // 1st day of the month offset, Monday = 0
  int   year;
};
RTC_DATA_ATTR struct time_struct now;

#define uS_TO_S_FACTOR 1000000  //Conversion factor for micro seconds to seconds

void setup(void)
{
  if (bootCount == 0) { // RGTODO: Add wakeup reason check also?
    pinMode(GREEN, OUTPUT);
    pinMode(BLUE, OUTPUT);
    pinMode(RED, OUTPUT);
    digitalWrite(BLUE, HIGH);
    digitalWrite(GREEN, HIGH);
    digitalWrite(RED, HIGH);
  }
  
  Serial.begin(115200);
  Serial.println();
  Serial.println("Paperd.ink");
  
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
  
  display.init(115200); // enable diagnostic output on Serial

  // clear the display
  display.fillScreen(GxEPD_WHITE);
  display.setRotation(0);
  /*
    //display status bar
    display.setFont(SECONDARY_FONT);
    display.setTextSize(1);
    display.setTextColor(GxEPD_BLACK);
    int16_t status_base_y = 0;
    int16_t status_base_x = display.width();
    if(wifi_connected){
    display.getTextBounds("192.168.9.27", status_base_x, status_base_y, &x1, &y1, &w, &h);
    display.setCursor(status_base_x-w-3, status_base_y+h);
    display.println("192.168.9.27");
    }
    int not_charging = analogRead(36);
    if (!not_charging) { //double negation
      //charging
    }
  */

  if((now.hour % 6 == 0 && now.min == 0) || bootCount == 0){
    // all WiFi related activity once every 6 hours
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
      configTime(0, 0, "pool.ntp.org");
    }
  }

  display_weather();

  // fill background
  display.fillRect(0, (display.height() / 3) * 2, display.width(), (display.height() / 3), GxEPD_BLACK);
  display.fillRect((display.width() / 4), 0, 5, (display.height() / 3) * 2, GxEPD_BLACK);
  display.fillRect((display.width() / 4), (display.height() / 3) * 2, 5, (display.height() / 3), GxEPD_WHITE);
  
  get_date_dtls(); // Get the date details before any time tasks
  display_calender();
  display_tasks();
  display_time(); // display time should be at the end as it controls the refresh type
  
  //display.powerDown();
  ++bootCount;
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Going to sleep...");
  esp_deep_sleep_start();
}

void loop()
{

}

// Display APIs
// display to-do list
void display_tasks(){
  
  int16_t  x1, y1;
  uint16_t w, h;
  if((now.hour % 6 == 0 && now.min == 0) || bootCount == 0){
  // Deserialize the JSON document
    DeserializationError error = deserializeJson(tasks, http_response);
  
    // Test if parsing succeeds.
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
    }else{
      Serial.println("Tasks:");
      for(uint8_t i = 0; i<todo_items_num; i++){
        const char* task = tasks[i]["content"];
        strncpy(todo_items[i],task,25);
        Serial.println(todo_items[i]);
      }
    }
  }
  
  display.setFont(SECONDARY_FONT);
  display.setTextColor(GxEPD_WHITE);
  int16_t td_list_base_y = 220;
  int16_t td_list_base_x = 115;
  display.setTextSize(1);

  display.setCursor(td_list_base_x, td_list_base_y);
  display.println("To-Do List");
  display.getTextBounds("To-Do List", td_list_base_x, td_list_base_y, &x1, &y1, &w, &h);
  for(uint8_t i = 0; i<todo_items_num; i++){
    display.setCursor(td_list_base_x, td_list_base_y + ((i + 1)*h) + ((i + 1) * 5));
    display.println(todo_items[i]);
  }
}

void display_weather(){
  int16_t weather_base_y = 135;
  int16_t weather_base_x = 25;
  if((now.hour % 6 == 0 && now.min == 0) || bootCount == 0){
    //update every 6 hours
    display.fillRect(weather_base_x - 5, weather_base_y - 5, 55 + 5, weather_base_y + 55 + 5, GxEPD_WHITE);
    const char* icon = fetch_weather();
    strcpy(weather_icon,icon);
  }
  drawBitmapFrom_SD_ToBuffer(SPIFFS, weather_icon, weather_base_x, weather_base_y, 0);
}

void display_time() {
  int16_t  x1, y1;
  uint16_t w, h;
  
  //display time
  display.setFont(PRIMARY_FONT);
  display.setTextSize(1);
  display.setTextColor(GxEPD_BLACK);
  int16_t time_base_y = 60;
  int16_t time_base_x = 25;
  display.getTextBounds("03", time_base_x, time_base_y, &x1, &y1, &w, &h); // 03 is arbitrary text to get the height and width
  display.fillRect(time_base_x - 10, time_base_y - h - 10, w + 15, time_base_y + h + 10, GxEPD_WHITE);

  display.setCursor(time_base_x, time_base_y);
  if (now.hour < 10) {
    display.print("0");
    display.print(now.hour);
  } else {
    display.println(now.hour);
  }

  display.setCursor(time_base_x, time_base_y + h + 10);
  if (now.min < 10) {
    display.print("0");
    display.print(now.min);
  } else {
    display.println(now.min);
  }

  if (!(now.min % 5) || bootCount == 0) {
    // Full update once every 5 mins and on the first boot
    display.update();
    delay(2000);
    display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, false);
  }else{
    display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, false);
    //display.updateWindow(time_base_x - 10, time_base_y - h - 10, w + 15, time_base_y + h + 10, false);
  }
}

void display_calender() {
  // display calender
  int16_t  x1, y1;
  uint16_t w, h;
  display.setFont(SECONDARY_FONT);
  display.setTextSize(1);
  display.setTextColor(GxEPD_BLACK);
  int16_t calender_base_y = 40;
  int16_t calender_base_x = 120;
  display.setCursor(calender_base_x, calender_base_y);
  display.println("Mon   Tue   Wed   Thu   Fri   Sat   Sun");
  display.getTextBounds("Mon   Tue   Wed   Thu   Fri   Sat   Sun", calender_base_x, calender_base_y, &x1, &y1, &w, &h); // need to consider partial string only because height will be same
  uint8_t num_offset, print_valid = 0;
  uint8_t day = 1;
  for (uint8_t j = 0; j <= 5; j++) {
    for (uint8_t i = 1; i <= 7 && day <= 31; i++) {
      // you can hack around with this value to align your text properly
      num_offset = 21; // 21 is what works for me for the first 2 columns
      if (i >= 3 && i <= 7) {
        num_offset = 17;    // then i need to reduce to 17
      }
      if (j == 0 && i == now.day_offset) {
        // start from the offset in the month, ie which day does 1st lie on
        print_valid = 1;
      }
      if (print_valid) {
        display.setCursor(calender_base_x + (i * (w / 7)) - num_offset, calender_base_y + ((j + 1)*h) + ((j + 1) * 7));
        if (day == now.mday) {
          char str[3];
          sprintf(str, "%d", day);
          int16_t  x2, y2;
          uint16_t w2, h2;
          display.getTextBounds(str, calender_base_x + (i * (w / 7)) - num_offset, calender_base_y + ((j + 1)*h) + ((j + 1) * 7), &x2, &y2, &w2, &h2);
          display.fillRect(x2 - 2, y2 - 2, w2 + 4, h2 + 4, GxEPD_BLACK);
          display.setTextColor(GxEPD_WHITE);
        } else {
          display.setTextColor(GxEPD_BLACK);
        }
        // once the offset is reached, start incrementing
        display.println(day);
        day += 1;
      }
    }
  }

  // display day
  display.setTextColor(GxEPD_WHITE);
  display.setFont(PRIMARY_FONT);
  display.setTextSize(1);
  display.setCursor(30, 250);
  display.println(now.mday);

  // display month
  display.setFont(SECONDARY_FONT);
  display.setTextSize(2);
  display.setCursor(30, 290);
  display.println(now.month);
}

static const uint16_t input_buffer_pixels = 20; // may affect performance

static const uint16_t max_palette_pixels = 256; // for depth <= 8

uint8_t input_buffer[3 * input_buffer_pixels]; // up to depth 24
uint8_t mono_palette_buffer[max_palette_pixels / 8]; // palette buffer for depth <= 8 b/w
uint8_t color_palette_buffer[max_palette_pixels / 8]; // palette buffer for depth <= 8 c/w

void drawBitmapFrom_SD_ToBuffer(fs::FS &fs, const char *filename, int16_t x, int16_t y, bool with_color)
{
  File file;
  bool valid = false; // valid format to be handled
  bool flip = true; // bitmap is stored bottom-to-top
  uint32_t startTime = millis();
  if ((x >= display.width()) || (y >= display.height())) return;
  Serial.println();
  Serial.print("Loading image '");
  Serial.print(filename);
  Serial.println('\'');

  file = fs.open(String("/") + filename, FILE_READ);
  if (!file)
  {
    Serial.print("File not found");
    return;
  }

  // Parse BMP header
  if (read16(file) == 0x4D42) // BMP signature
  {
    uint32_t fileSize = read32(file);
    uint32_t creatorBytes = read32(file);
    uint32_t imageOffset = read32(file); // Start of image data
    uint32_t headerSize = read32(file);
    uint32_t width  = read32(file);
    uint32_t height = read32(file);
    uint16_t planes = read16(file);
    uint16_t depth = read16(file); // bits per pixel
    uint32_t format = read32(file);
    if ((planes == 1) && ((format == 0) || (format == 3))) // uncompressed is handled, 565 also
    {
      Serial.print("File size: "); Serial.println(fileSize);
      Serial.print("Image Offset: "); Serial.println(imageOffset);
      Serial.print("Header size: "); Serial.println(headerSize);
      Serial.print("Bit Depth: "); Serial.println(depth);
      Serial.print("Image size: ");
      Serial.print(width);
      Serial.print('x');
      Serial.println(height);
      // BMP rows are padded (if needed) to 4-byte boundary
      uint32_t rowSize = (width * depth / 8 + 3) & ~3;
      if (depth < 8) rowSize = ((width * depth + 8 - depth) / 8 + 3) & ~3;
      if (height < 0)
      {
        height = -height;
        flip = false;
      }
      uint16_t w = width;
      uint16_t h = height;
      if ((x + w - 1) >= display.width())  w = display.width()  - x;
      if ((y + h - 1) >= display.height()) h = display.height() - y;
      valid = true;
      uint8_t bitmask = 0xFF;
      uint8_t bitshift = 8 - depth;
      uint16_t red, green, blue;
      bool whitish, colored;
      if (depth == 1) with_color = false;
      if (depth <= 8)
      {
        if (depth < 8) bitmask >>= depth;
        file.seekSet(54); //palette is always @ 54
        for (uint16_t pn = 0; pn < (1 << depth); pn++)
        {
          blue  = file.read();
          green = file.read();
          red   = file.read();
          file.read();
          whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
          colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
          if (0 == pn % 8) mono_palette_buffer[pn / 8] = 0;
          mono_palette_buffer[pn / 8] |= whitish << pn % 8;
          if (0 == pn % 8) color_palette_buffer[pn / 8] = 0;
          color_palette_buffer[pn / 8] |= colored << pn % 8;
        }
      }
      //display.fillScreen(GxEPD_WHITE);
      uint32_t rowPosition = flip ? imageOffset + (height - h) * rowSize : imageOffset;
      for (uint16_t row = 0; row < h; row++, rowPosition += rowSize) // for each line
      {
        uint32_t in_remain = rowSize;
        uint32_t in_idx = 0;
        uint32_t in_bytes = 0;
        uint8_t in_byte = 0; // for depth <= 8
        uint8_t in_bits = 0; // for depth <= 8
        uint16_t color = GxEPD_BLACK;
        file.seekSet(rowPosition);
        for (uint16_t col = 0; col < w; col++) // for each pixel
        {
          // Time to read more pixel data?
          if (in_idx >= in_bytes) // ok, exact match for 24bit also (size IS multiple of 3)
          {
            in_bytes = file.read(input_buffer, in_remain > sizeof(input_buffer) ? sizeof(input_buffer) : in_remain);
            in_remain -= in_bytes;
            in_idx = 0;
          }
          switch (depth)
          {
            case 24:
              blue = input_buffer[in_idx++];
              green = input_buffer[in_idx++];
              red = input_buffer[in_idx++];
              whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
              colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
              break;
            case 16:
              {
                uint8_t lsb = input_buffer[in_idx++];
                uint8_t msb = input_buffer[in_idx++];
                if (format == 0) // 555
                {
                  blue  = (lsb & 0x1F) << 3;
                  green = ((msb & 0x03) << 6) | ((lsb & 0xE0) >> 2);
                  red   = (msb & 0x7C) << 1;
                }
                else // 565
                {
                  blue  = (lsb & 0x1F) << 3;
                  green = ((msb & 0x07) << 5) | ((lsb & 0xE0) >> 3);
                  red   = (msb & 0xF8);
                }
                whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0)); // reddish or yellowish?
              }
              break;
            case 1:
            case 4:
            case 8:
              {
                if (0 == in_bits)
                {
                  in_byte = input_buffer[in_idx++];
                  in_bits = 8;
                }
                uint16_t pn = (in_byte >> bitshift) & bitmask;
                whitish = mono_palette_buffer[pn / 8] & (0x1 << pn % 8);
                colored = color_palette_buffer[pn / 8] & (0x1 << pn % 8);
                in_byte <<= depth;
                in_bits -= depth;
              }
              break;
          }
          if (whitish)
          {
            color = GxEPD_WHITE;
          }
          else if (colored && with_color)
          {
            color = GxEPD_RED;
          }
          else
          {
            color = GxEPD_BLACK;
          }
          uint16_t yrow = y + (flip ? h - row - 1 : row);
          display.drawPixel(x + col, yrow, color);
        } // end pixel
      } // end line
      Serial.print("loaded in "); Serial.print(millis() - startTime); Serial.println(" ms");
    }
  }
  file.close();
  if (!valid)
  {
    Serial.println("bitmap format not handled.");
  }
}

void drawBitmapFromSD(fs::FS &fs, const char *filename, int16_t x, int16_t y, bool with_color)
{
  drawBitmapFrom_SD_ToBuffer(fs, filename, x, y, with_color);
  display.update();
}

uint16_t read16(File& f)
{
  // BMP data is stored little-endian, same as Arduino.
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File& f)
{
  // BMP data is stored little-endian, same as Arduino.
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

// Time APIs
int8_t get_date_dtls() {
  // Set timezone to Indian Standard Time
  setenv("TZ", "UTC-05:30", 1);
  
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return -1;
  }
  time_t epoch = mktime(&timeinfo);

  sscanf(ctime(&epoch), "%s %s %hhd %hhd:%hhd:%hhd %d", now.wday, now.month, &now.mday, &now.mil_hour, &now.min, &now.sec, &now.year);
  
  now.month_num = timeinfo.tm_mon + 1;
  // gives offset of first day of the month with respect to Monday
  //https://www.tondering.dk/claus/cal/chrweek.php#calcdow
  // 1=Monday to 7=Sunday
  uint8_t a = (14 - now.month_num) / 12;
  uint16_t y = now.year - a;
  uint16_t m = now.month_num + (12 * a) - 2;
  // change +7 at the end to whatever first day of week you want.
  // But change the header '"Mon   Tue   Wed   Thu   Fri   Sat   Sun"' as well above.
  // currently +7 => Monday
  // +1 => Sunday
  now.day_offset = ((1 + y + (y / 4) - (y / 100) + (y / 400) + ((31 * m) / 12)) % 7) + 7;

  // convert to 12 hour
  if (now.mil_hour > 12) {
    now.hour = now.mil_hour - 12;
  } else {
    now.hour = now.mil_hour;
  }

  Serial.printf("Time is %d %d:%d:%d on %s on the %d/%d/%d . It is a %s. day_offset: %d",now.mil_hour,now.hour,now.min,now.sec,now.wday,now.mday,now.month_num,now.year, now.month, now.day_offset);
  return 0;
}

// Fetch tasks
int handle_get_response(struct sh2lib_handle *handle, const char *data, size_t len, int flags)
{
    if (len > 0) {
        //Serial.printf("%.*s\n", len, data);
        memcpy(&http_response[resp_pointer],data,tasks_size);
        resp_pointer += len;
    }

    if (flags == DATA_RECV_RST_STREAM) {
        request_finished = true;
        Serial.println("STREAM CLOSED");
        memcpy(&http_response[resp_pointer],"\0",len);
        Serial.printf("%.*s\n",resp_pointer+1,http_response);
    }
    return 0;
}

void todo_task(void *args)
{
  struct sh2lib_handle hd;

  if (sh2lib_connect(&hd, "https://api.todoist.com") != ESP_OK) {
    Serial.println("Error connecting to todoist server");
    vTaskDelete(NULL);
  }

  Serial.println("Connected");

  const nghttp2_nv nva[] = { SH2LIB_MAKE_NV(":method", "GET"),
                             SH2LIB_MAKE_NV(":scheme", "https"),
                             SH2LIB_MAKE_NV(":authority", hd.hostname),
                             SH2LIB_MAKE_NV(":path", "/rest/v1/tasks"),
                             SH2LIB_MAKE_NV("Authorization", todoist_token)
                           };

  sh2lib_do_get_with_nv(&hd, nva, sizeof(nva) / sizeof(nva[0]), handle_get_response);

  while (1) {

    if (sh2lib_execute(&hd) != ESP_OK) {
      Serial.println("Error in send/receive");
      break;
    }

    if (request_finished) {
      break;
    }
    vTaskDelay(10);
  }

  sh2lib_free(&hd);
  Serial.println("Disconnected");
  
  vTaskDelete(NULL);
}

// fetch weather
const char* fetch_weather(){
  HTTPClient http;
  http.begin(openweathermap_link);
  int httpCode = http.GET();
  
  if (httpCode > 0) { //Check for the returning code
          String payload = http.getString();
          Serial.println(httpCode);
          Serial.println(payload);
          DeserializationError error = deserializeJson(weather_json, payload);
          // Test if parsing succeeds.
          if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
            http.end(); //Free the resources
            return "Error.bmp";
          }
  }else {
        Serial.println("Error on HTTP request");
        http.end(); //Free the resources
        return "Error.bmp";
  }
  http.end(); //Free the resources
  
  const char* weather_string = weather_json["weather"][0]["main"];
  if(!strcmp(weather_string,"Drizzle") || !strcmp(weather_string,"Rain") || !strcmp(weather_string,"Thunderstorm")){
    Serial.println("Displaying Drizzle.bmp");
    return "Drizzle.bmp";
  }else if(!strcmp(weather_string,"Clouds")){
    Serial.println("Displaying Clouds.bmp");
    return "Clouds.bmp";
  }else if(!strcmp(weather_string,"Clear")){
    Serial.println("Displaying Clear.bmp");
    return "Clear.bmp";
  }else{
    return "Error.bmp";
  }
}

// WiFi API
int8_t Start_WiFi(const char* ssid, const char* password){
 int connAttempts = 0;
 Serial.println("\r\nConnecting to: "+String(ssid));
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED ) {
   delay(500);
   Serial.print(".");
   if(connAttempts > 20) return -1;
   connAttempts++;
 }
 Serial.println("");
 return 0;
}
