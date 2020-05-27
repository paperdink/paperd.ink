

#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
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

#include "date_time.h"

RTC_DATA_ATTR struct time_struct now; // keep track of time
String time_zone_base = "UTC";

// Time APIs
int8_t get_date_dtls(String time_zone) {
  String time_zone_string = time_zone_base+time_zone;
  setenv("TZ", time_zone_string.c_str(), 1);
  
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
  now.day_offset = (((1 + y + (y / 4) - (y / 100) + (y / 400) + ((31 * m) / 12)) % 7) + 7) % 7;

  // convert to 12 hour
  if (now.mil_hour > 12) {
    now.hour = now.mil_hour - 12;
  } else {
    now.hour = now.mil_hour;
  }

  Serial.printf("Time is %d %d:%d:%d on %s on the %d/%d/%d . It is the month of %s. day_offset: %d\n",now.mil_hour,now.hour,now.min,now.sec,now.wday,now.mday,now.month_num,now.year, now.month, now.day_offset);
  return 0;
}

int8_t set_time() {

  struct tm t;
  t.tm_year = 2020 - 1900;
  t.tm_mon = 2 - 1;         // Month, 1 - jan to 12 - dec
  t.tm_mday = 3;          // Day of the month
  t.tm_hour = 9;
  t.tm_min = 57;
  t.tm_sec = 0;
  t.tm_isdst = -1;        // Is DST on? 1 = yes, 0 = no, -1 = unknown

  time_t epoch;
  epoch = mktime(&t);

  struct timeval now;
  now.tv_sec = epoch;
  now.tv_usec = 0;

  struct timezone tz = {-330, 0};
  return settimeofday(&now, &tz);

}
