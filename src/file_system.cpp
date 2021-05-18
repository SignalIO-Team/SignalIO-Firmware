#include "file_system.h"


StaticJsonDocument<1024> FileSystem::get_config(const char* path){

  StaticJsonDocument<1024> json_config;
  File configFile = SPIFFS.open(path, "r");
  if(!configFile){
    Serial.println("failed to load JSON config"); // debug message
    configFile.close();
    return json_config;
  }
  size_t size = configFile.size();
  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);
  deserializeJson(json_config, buf.get());

  return json_config;
}


bool FileSystem::read_file(const char* path){
    File file = SPIFFS.open(path, "r");
    size_t size = file.size();
    
    if(!file){
        Serial.println("Error opening file: " + String(path));
        return false;
    }
    Serial.println("Config file exist: " + String(path));
    Serial.printf("File size: %i (bytes)\n", size);
    file.close();
    return true;
}


bool FileSystem::write_file(const char* path, char buff[]){
    File f = SPIFFS.open(path, "w");
    size_t size = f.size();

    if(!f){
        f.close();
        return false;
    }

    if(strlen(buff) == 0){ // needs debug
        return false;
    }

    if(strlen(buff) > buff_size){ // needs debugg
        return false;
    }

    if (size > buff_size)
    {
        f.close();
        return false;
    }

    if(f.print(buff)){
        return true;
    }

    return false;   
}


bool FileSystem::config_reset(const char* path, int key){
    StaticJsonDocument<20> config_rst;
    
    File config = SPIFFS.open(path, "w");

    if(!config){
      Serial.printf("File does not exist!\n");
      config.close();
      return false;
    }
    switch (key)
        {
        case 0:
            config_rst["config_flag"] = 1;
            serializeJson(config_rst, config);
            break;
        
        case 1:
            config_rst["ssid"] = "0";
            config_rst["password"] = "0";
            serializeJson(config_rst, config);
            break;

        case 2:
            config_rst["state"] = "0";
            serializeJson(config_rst, config);
            break;

        default:
            Serial.println("default erase");
            break;
        }

    return true;
}