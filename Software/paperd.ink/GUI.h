
#ifndef GUI_H
#define GUI_H

#include <GxEPD.h>
#include <GxGDEW042T2/GxGDEW042T2.h>      // 4.2" b/w
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#include "FS.h"
#include "SPIFFS.h"
#include "SD.h"

void display_config_gui(GxEPD_Class* display);
void display_tasks(GxEPD_Class* display);
void display_weather(GxEPD_Class* display);
void display_calender(GxEPD_Class* display);
void display_time(GxEPD_Class* display);
void display_battery(GxEPD_Class* display, float batt_voltage, uint8_t not_charging);
void display_wifi(GxEPD_Class* display, uint8_t status);
void display_background(GxEPD_Class* display);
void display_update(GxEPD_Class* display);

void todo_task(void *args);

void drawBitmapFrom_SD_ToBuffer(GxEPD_Class* display, fs::FS &fs, const char *filename, int16_t x, int16_t y, bool with_color);

#endif /* GUI_H */
