#include <Arduino.h>
#include <Pangodream_18650_CL.h>
#include "json_lib.h"
#include "file_system.h"
#include "sensors.h"
#include "wifi_manager.h"
#include "mqtt.h"
#include "config_menu.h"
#include "json_msg_packer.h"
#include "system_codes.h"
#include "deep_sleep.h"

RTC_DATA_ATTR int boot_num;

uint8_t SIGNAL_LED = 19;
uint8_t CHARGE_MONITORING_ADC_PIN = 34;
uint8_t FACT_RESET_BTN = 23;

String SoC = "ESP32-WROOM";
String firmware_version = "1.0.0";

uint8_t sensor_select;

bool debug_state;
bool config_state;
bool dht_init;
bool sensors_init;
bool state;
bool deep_sleep_flag;
bool pwr_monitor_flag;
bool relay_pin = false;

String service_topic;
String error_topic;
                 
const char* config_path = "/config.json";
const char* wifi_path = "/wifi_creds.json";
const char* actuator_state_path = "/actuator_state.json";

unsigned int deep_sleep_timer;
int deep_sleep_mode;

FileSystem fileSystem;
sensors sensor; 
WifiManager WifiConn;
mqtt MQTT;
MessagePacker packer;
Pangodream_18650_CL bat;
Sleep slp;

StaticJsonDocument<1024> config;

void get_system_info(){
  Serial.print("SoC: ");
  Serial.println(SoC);
  Serial.printf("Firmware size: %i\n",ESP.getSketchSize());
  Serial.printf("Free space for firmware: %i\n",ESP.getFreeSketchSpace());
  Serial.printf("Heap size: %i\n",ESP.getHeapSize());
  Serial.printf("Free heap: %i\n",ESP.getFreeHeap());
  Serial.print("SDK version: ");
  Serial.println(ESP.getSdkVersion());
}


void send_systme_info(){
    DynamicJsonDocument sys_info_msg_container(255);
    DynamicJsonDocument sys_info_data_container(255);

    JsonObject inf_msg = sys_info_data_container.to<JsonObject>();

    inf_msg["SoC"] = SoC;
    inf_msg["Firmware size"] = String(ESP.getSketchSize()).c_str();
    inf_msg["Free firmware space"] = String(ESP.getFreeSketchSpace()).c_str();
    inf_msg["Heap size"] = String(ESP.getHeapSize()).c_str();
    inf_msg["Free heap"] = String(ESP.getFreeHeap()).c_str();
    inf_msg["SDK version"] = String(ESP.getSdkVersion()).c_str();

    JsonArray data = sys_info_msg_container.createNestedArray("props");
    char info_msg_buff[255];
    data.add(inf_msg);
    serializeJsonPretty(sys_info_msg_container, info_msg_buff);
}


void load_params(){
  MQTT.device_id = config["alias"];
  MQTT.mqttUser = config["mqtt_user"];
  MQTT.mqttPassword = config["mqtt_pass"];
  MQTT.mqttServer = config["mqtt_brocker"];
  MQTT.topic = config["mqtt_topic"];
  MQTT.callback_msg = config["mqtt_callback"];
  MQTT.sensor_port = atoi(config["module_port"]);
  MQTT.save_actuator_state_flag = atoi(config["save_actuator_state"]);
  MQTT.mqtt_port = atoi(config["mqtt_port"]);

  sensor.module_pin = atoi(config["module_port"]);
}


void signal_led(int time){
  digitalWrite(SIGNAL_LED, HIGH);
  delay(time);
  digitalWrite(SIGNAL_LED, LOW);
  delay(time);
}


void deep_sleep_wakeup_reason(){
  const char* sensor_message = config["mqtt_callback"];
  
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 :  
            MQTT.mqtt_pub(packer.pack(sensor_message, "digital_sensor", STRING), config["mqtt_topic"]); 
            break;
    case ESP_SLEEP_WAKEUP_EXT1 : 
            Serial.println("Wakeup caused by external signal using RTC_CNTL"); 
            break;
    case ESP_SLEEP_WAKEUP_TIMER: 
            Serial.println("Wakeup caused by timer"); 
            MQTT.mqtt_pub(packer.message("wake up with timer", "message"), service_topic.c_str());
            break;
    default : 
            Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); 
            break;
  }
}


void sys_reset(){
  int state = digitalRead(FACT_RESET_BTN);
  if(state == LOW){
     bool sys_reset_flag = fileSystem.config_reset(config_path, 0);
     //bool wifi_reset_flag = fileSystem.config_reset(wifi_path, 1);
     bool actuator_state_reset_flag = fileSystem.config_reset(actuator_state_path, 2);

     if(sys_reset_flag && actuator_state_reset_flag){
       Serial.println("Config erased");
       for (size_t i = 0; i < 3; i++)
       {
         signal_led(1000);
       }
       ESP.restart();
     }
  }
}


//TODO -- test/calibrate
void pwr_manager(){
  analogRead(CHARGE_MONITORING_ADC_PIN);
  int battery_charge_level = bat.getBatteryChargeLevel();
  int battery_charge_volts = bat.getBatteryVolts();
  Serial.print("Battery voltage: ");
  Serial.println(battery_charge_volts);
  Serial.print("Battery charge level: ");
  Serial.println(battery_charge_level);
  MQTT.mqtt_pub(packer.pack(String(battery_charge_level).c_str(), "battery_charge_level", STRING), service_topic.c_str());
  delay(1000);
}


void hall_sensor(){
  int hall_val = hallRead();
  MQTT.mqtt_pub(packer.pack(String(hall_val).c_str(), "hall sensor", STRING), config["mqtt_topic"]);
}


void tmp36_sensor(){
  float temperature_reading = sensor.tmp36_read_temperature();
  MQTT.mqtt_pub(packer.pack(String(temperature_reading).c_str(), "tmp36 temperature sensor", STRING), config["mqtt_topic"]);
}


void pir_motion_sensor(){
    if(sensors_init){
    int res = sensor.digital_sensor_read();
    if(res == HIGH){
      if(!state){
        MQTT.mqtt_pub(packer.pack("on", "PIR_sensor", STRING), config["mqtt_topic"]);
        MQTT.mqtt_sub();
        signal_led(10);
      }
      state = true;
    }
    else
    {
      state = false;
      MQTT.mqtt_pub(packer.pack("off", "PIR_sensor", STRING), config["mqtt_topic"]);
      MQTT.mqtt_sub();
      delay(100);
    }
  }
  else
  {
    Serial.println("PIR sensor calibration");
    digitalWrite(SIGNAL_LED, HIGH);
    sensor.sensor_init();
    sensors_init = true;
    Serial.println("PIR sensor calibration done!\n"); //Debug msg
    digitalWrite(SIGNAL_LED, LOW);
  }
  MQTT.mqtt_sub();
}


void digital_sensor(){
    int res = sensor.digital_sensor_read();
    if(res == LOW){
        MQTT.mqtt_pub(packer.pack("on", "digital_sensor", STRING), config["mqtt_topic"]);
        MQTT.mqtt_sub();
        signal_led(10);
    }
    if(res != LOW){
      MQTT.mqtt_pub(packer.pack("off", "digital_sensor", STRING), config["mqtt_topic"]);
      MQTT.mqtt_sub();
      signal_led(10);
    }
    MQTT.mqtt_sub();
}


void analog_sensor(){
    int analog_sensor_message = sensor.analog_sensor_read();
    Serial.println(analog_sensor_message);
    MQTT.mqtt_pub(packer.pack(String(analog_sensor_message).c_str(), "analog sensor", STRING), config["mqtt_topic"]);
    MQTT.mqtt_sub();
    signal_led(10);
}


void relay(){
 if(relay_pin){
    MQTT.mqtt_sub();
  }
  else
  {
    MQTT.topic_sub();
    bool actuator_save_flag = atoi(config["save_actuator_state"]);
    if(actuator_save_flag){
      sensor.relay_init();
    }
    if(!actuator_save_flag){
      int module_pin = atoi(config["module_port"]);
      pinMode(module_pin, OUTPUT_OPEN_DRAIN);
      digitalWrite(module_pin, HIGH);
    }
    relay_pin = true;
  }
}


void deepsleep_mode(){
  Serial.println("Enable deep sleep mode");
    deep_sleep_mode = atoi(config["deepsleep_type"]);
    Serial.println(deep_sleep_mode);
    if(deep_sleep_mode == TMR_MODE){
        
        // debug start
        // ++boot_num;
        // debug end

        Serial.println("timer mode");
        deep_sleep_timer = atoi(config["deepseep_tmr"]);

        sys_reset();

        switch (sensor_select)
        {
        case DIGITAL_SENSOR:
          pwr_manager();
          sensor.sensor_init();
          delay(1000);
          digital_sensor();
          slp.tmr_sleep(deep_sleep_timer);
          // debug start
          // MQTT.mqtt_pub(packer.message(String(boot_num).c_str(), STRING), service_topic.c_str());
          //debug end
          break;

        case ANALOG_SENSOR:
          pwr_manager();
          delay(1000);
          analog_sensor();
          slp.tmr_sleep(deep_sleep_timer);
          break;

        case TMP36_SENSOR:
          pwr_manager();
          delay(1000);
          tmp36_sensor();
          slp.tmr_sleep(deep_sleep_timer);
          break;

        default:
          Serial.println("module not supported by deep sleep mode"); // debug
          MQTT.mqtt_pub(packer.message(MODULE_NOT_SUPPORTED_BY_DEEPSLEEP, "error"), service_topic.c_str());
          signal_led(1000);
          break;
        }
    }

    if(deep_sleep_mode == PIN_TRIGGER_MODE){
        pwr_manager();
        deep_sleep_wakeup_reason();
        Serial.println("trigger mode\n For now only works on pin 14");
        pinMode(14, OUTPUT_OPEN_DRAIN);
        digitalWrite(14, LOW);
        slp.pin_trigger_sleep();
    }
}


void setup()
{
  pinMode(SIGNAL_LED, OUTPUT);
  pinMode(FACT_RESET_BTN, INPUT_PULLUP);

  for (size_t i = 0; i < 5; i++)
  {
    signal_led(500);
  }

  Serial.begin(9600);
  get_system_info();

  if(!SPIFFS.begin()){
    Serial.println("Failed to init file system");
    signal_led(5000);
    ESP.restart();
  }

  Serial.println("File system inited");

  bool file_state = fileSystem.read_file("/config.json");
  sys_reset();

  if(file_state){
    config = fileSystem.get_config(config_path);
    config_state = config["config_flag"];
    Serial.println("System init...");
    bool state = WifiConn.wifi_connect();
    if(state){
        digitalWrite(SIGNAL_LED, HIGH);
        Serial.println("Wi-Fi connected");
        WifiConn.printCurrentNet();
        WifiConn.printWifiData();
  
        if(config_state){
          Serial.println("Config server started\nUse your local IP to connect");
          config_menu();
        }

        load_params();
        deep_sleep_flag = atoi(config["deepsleep_flag"]);
        bool mqtt_state = MQTT.mqtt_connect();
        if(mqtt_state){
          Serial.println("MQTT connected!");
        }
        else
        {
          Serial.println("MQTT error. Not connected");
        }    
    }    
    else
    {
      Serial.println("Wifi not connected");
      Serial.println("Start scanning...");
      WifiConn.scan_networks();
      WifiConn.manager();
    }

    sensor_select = atoi(config["module"]);
    Serial.printf("Module: %x\n", sensor_select);
  }

  if(!file_state){
    Serial.println("File not found\n Mount SPIFFS File system and upload configs");
    for (size_t i = 0; i <5; i++)
    {
      signal_led(500);
    }
  }

  digitalWrite(SIGNAL_LED, LOW);

  MQTT.topic_sub();
  const char* alias_dev = config["alias"];
  pwr_monitor_flag = atoi(config["pwr_monitor_flag"]);

  service_topic = "device/" + String(alias_dev) + "/system_message";
  error_topic = "device/" + String(alias_dev) + "/error";

  Serial.println("Configureation Done");
  
  if(deep_sleep_flag){
    deepsleep_mode();
  }

  send_systme_info();
}


void loop() 
{
  sys_reset();

  if(pwr_monitor_flag){
    pwr_manager();
  }


  switch (sensor_select)
  {
  case TMP36_SENSOR:
    tmp36_sensor();
    break;
  
  case DIGITAL_SENSOR:
    if(!sensors_init){
        Serial.println("Digital sensor calibration"); //Debug msg
        digitalWrite(SIGNAL_LED, HIGH);
        sensor.sensor_init();
        sensors_init = true;
        Serial.println("Digital sensor calibration done!"); //Debug msg
        digitalWrite(SIGNAL_LED, LOW);
      }
    digital_sensor();
    break;

  case ANALOG_SENSOR:
    analog_sensor();
    break;

  case PIR_SENSOR:
    pir_motion_sensor();
    break;

  case RELAY:
    relay();
    break;

  case HALL_SENSOR:
    hall_sensor();
    break;
    
  default:
    Serial.println("No module selected"); // debug
    MQTT.mqtt_pub(packer.message(MODULE_NOT_FOUND, "error"), service_topic.c_str());
    signal_led(1000);
    break;
  }
}