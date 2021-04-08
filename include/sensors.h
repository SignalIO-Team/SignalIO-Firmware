#include <Arduino.h>
#include <DHT.h>
#include <DHT_U.h>
#include "DHT.h"
#include "file_system.h"

#ifndef SENSORS_H
#define SENSORS_H

#define DHTTYPE DHT11

#define TMP36_SENSOR 0xC7
#define DIGITAL_SENSOR 0xC6
#define ANALOG_SENSOR 0xC5
#define RELAY 0xC4
#define PIR_SENSOR 0xC3
#define HALL_SENSOR 0xC2

class sensors
{
private:
    const int calibration_time = 20;
    const int temp_conversion_factor = 100;
    const int adc_resolution = 4095;
    const float adc_vref = 3.3;
    const float temp_offset = 0.5;

public:

    uint8_t module_pin;

    //tmp36 sensor
    float tmp36_read_temperature(void);

    //simple sensor interface
    void sensor_init(void);
    int digital_sensor_read(void);
    int analog_sensor_read(void);
    void relay_init(void);
};

#endif