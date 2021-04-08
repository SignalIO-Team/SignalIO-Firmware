#include "sensors.h"

FileSystem actuator_file_system;


float sensors::tmp36_read_temperature(){
    float adc_reading = analogRead(module_pin);
    float voltage_conversion = (adc_reading * adc_vref) / adc_resolution;
    return (voltage_conversion - temp_offset) * temp_conversion_factor;
}


void sensors::sensor_init(){
    pinMode(module_pin, INPUT);
    pinMode(14, OUTPUT_OPEN_DRAIN);
    digitalWrite(14, LOW);
}


int sensors::digital_sensor_read(){
    int state = digitalRead(module_pin);
    return state;
}


int sensors::analog_sensor_read(){
    int state = analogRead(module_pin);
    return state;
}


void sensors::relay_init(){
    StaticJsonDocument<255> actuator_status;
    
    actuator_status = actuator_file_system.get_config("/actuator_state.json");
    
    int actuator_state = atoi(actuator_status["state"]);

    pinMode(module_pin, OUTPUT_OPEN_DRAIN);
    
    switch (actuator_state)
    {
    case 1:
        digitalWrite(module_pin, LOW);
        break;
    
    case 0:
        digitalWrite(module_pin, HIGH);
        break;
    
    default:
        digitalWrite(module_pin, HIGH);
        break;
    }
}