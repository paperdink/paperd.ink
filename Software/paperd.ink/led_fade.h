
#ifndef LED_FADE_H
#define LED_FADE_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LEDC_CHANNEL_R     0
#define LEDC_CHANNEL_G     1
#define LEDC_CHANNEL_B     2

#define LEDC_TIMER_RES  8
#define LEDC_BASE_FREQ     12000

void start_led_fade(uint8_t hue);
void stop_led_fade();

extern uint8_t brightness;
extern uint8_t fadeAmount;

#endif /*LED_FADE_H*/
