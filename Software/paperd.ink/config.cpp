#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

#include "config.h"

int8_t deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(fs.remove(path)){
        Serial.println("- file deleted");
        return 0;
    }

    Serial.println("- delete failed");
    return -1;
}

int8_t readFile(fs::FS &fs, const char* path, char* buf){
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return -1;
    }

    Serial.println("- read from file:");
    uint32_t i = 0;
    while(file.available()){
        buf[i++] = file.read();
    }
    buf[i] = '\0';
    Serial.println(buf);
    return 0;
}

int8_t writeFile(fs::FS &fs, const char* path, const char* message){
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return -1;
    }
    if(file.print(message)){
        Serial.println("- file written");
        return 0;
    }
    Serial.println("- write failed");
    return -1;
}

int8_t save_config(const char* config){
  return writeFile(SPIFFS,"/config.txt",config);
}

int8_t load_config(){
  if(readFile(SPIFFS,"/config.txt",buf) < 0){
    return -1;
  }
  StaticJsonDocument<2048>config;
  DeserializationError error = deserializeJson(config, buf);
  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed for config: "));
    Serial.println(error.c_str());
    return -2;
  }
  Serial.println("Got config details");
  
  strncpy(city_string,config["city"],30);
  strncpy(country_string,config["country"],30);
  strncpy(todoist_token_string,config["todoist_token"],42);
  strncpy(openweather_appkey_string,config["openweather_appkey"],34);
  strncpy(time_zone_string,config["timezone"],7);
  config_done = config["config_done"];
}
