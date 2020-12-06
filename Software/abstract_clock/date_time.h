
#ifndef TIME_H
#define TIME_H

#include <stdint.h>
#include <esp_attr.h>

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

extern RTC_DATA_ATTR struct time_struct now;

//Conversion factor for micro seconds to seconds
#define uS_TO_S_FACTOR 1000000ULL

int8_t get_date_dtls(String time_zone);

#endif /* TIME_H */
