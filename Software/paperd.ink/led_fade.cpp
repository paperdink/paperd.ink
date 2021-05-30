
#include "Arduino.h"
#include "esp32-hal-ledc.h"
#include "config.h"
#include "led_fade.h"

uint8_t brightness = 0;    // how bright the LED is
uint8_t fadeAmount = 5;    // how many points to fade the LED by
TaskHandle_t wifi_fade_handle = NULL;

// LED fade APIs

// http://www.instructables.com/id/How-to-Use-an-RGB-LED/?ALLSTEPS
void hueToRGB(const uint8_t hue, uint8_t brightness, uint32_t* R, uint32_t* G, uint32_t* B)
{
    uint16_t scaledHue = (hue * 6);
    uint8_t segment = scaledHue / 256; // segment 0 to 5 around the
                                            // color wheel
    uint16_t segmentOffset = scaledHue - (segment * 256); // position within the segment

    uint8_t complement = 0;
    uint16_t prev = (brightness * ( 255 -  segmentOffset)) / 256;
    uint16_t next = (brightness *  segmentOffset) / 256;

    brightness = 255 - brightness;
    complement = 255;
    prev = 255 - prev;
    next = 255 - next;

    switch(segment ) {
    case 0:      // red
        *R = brightness;
        *G = next;
        *B = complement;
    break;
    case 1:     // yellow
        *R = prev;
        *G = brightness;
        *B = complement;
    break;
    case 2:     // green
        *R = complement;
        *G = brightness;
        *B = next;
    break;
    case 3:    // cyan
        *R = complement;
        *G = prev;
        *B = brightness;
    break;
    case 4:    // blue
        *R = next;
        *G = complement;
        *B = brightness;
    break;
   case 5:      // magenta
    default:
        *R = brightness;
        *G = complement;
        *B = prev;
    break;
    }
}

void led_fade(void *pvParameters){
  uint8_t* hue_p = (uint8_t*)pvParameters;
  uint8_t hue = *hue_p;
  //Serial.printf("RGDEBUG LED fade with hue: %d hue_p: %d addres: %d\n",hue,*hue_p,hue_p);
  uint8_t brightness = 0;
  uint32_t R = 0;
  uint32_t G = 0;
  uint32_t B = 0;
  
  //Serial.printf("RGDEBUG LED fade with hue: %d hue_p: %d addres: %d\n",hue,*hue_p,hue_p);
  while(1){
    hueToRGB(hue, brightness, &R, &G, &B);
    brightness = brightness + fadeAmount;
    if (brightness <= 0 || brightness >= 180) {
      fadeAmount = -fadeAmount;
    }
    ledcWrite(LEDC_CHANNEL_R, R);
    ledcWrite(LEDC_CHANNEL_G, G);
    ledcWrite(LEDC_CHANNEL_B, B);
    vTaskDelay((TickType_t) 20 / portTICK_PERIOD_MS);
  }
}

void start_led_fade(uint8_t hue){
  pinMode(BLUE_LED_PIN,OUTPUT);
  pinMode(GREEN_LED_PIN,OUTPUT);
  pinMode(RED_LED_PIN,OUTPUT);
  digitalWrite(BLUE_LED_PIN, HIGH);
  digitalWrite(GREEN_LED_PIN, HIGH);
  digitalWrite(RED_LED_PIN, HIGH);
      
  // Setup timer and attach timer to a led pin
  ledcSetup(LEDC_CHANNEL_R, LEDC_BASE_FREQ, LEDC_TIMER_RES);
  ledcSetup(LEDC_CHANNEL_G, LEDC_BASE_FREQ, LEDC_TIMER_RES);
  ledcSetup(LEDC_CHANNEL_B, LEDC_BASE_FREQ, LEDC_TIMER_RES);
  
  ledcAttachPin(RED_LED_PIN, LEDC_CHANNEL_R);
  ledcAttachPin(GREEN_LED_PIN, LEDC_CHANNEL_G);
  ledcAttachPin(BLUE_LED_PIN, LEDC_CHANNEL_B);
  //Serial.printf("RGDEBUG LED fade with hue: %d address: %d\n",hue,&hue);
  xTaskCreatePinnedToCore(
       led_fade
    ,  "LEDFade"    // A name just for humans
    ,  2048         // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  &hue
    ,  3            // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  &wifi_fade_handle 
    ,  1);  //RGTODO: Check which core is to be used
}

void stop_led_fade(){
  if(wifi_fade_handle != NULL){
    vTaskDelete(wifi_fade_handle);
  }
  wifi_fade_handle = NULL;
  ledcDetachPin(RED_LED_PIN);
  ledcDetachPin(GREEN_LED_PIN);
  ledcDetachPin(BLUE_LED_PIN);
  digitalWrite(BLUE_LED_PIN, HIGH);
  digitalWrite(GREEN_LED_PIN, HIGH);
  digitalWrite(RED_LED_PIN, HIGH);
}
