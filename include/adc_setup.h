#ifndef ADC_SETUP_H
#define ADC_SETUP_H

#include <Arduino.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>

#define battery_channel ADC1_CHANNEL_6
#define battery_adc_pin ADC1_CHANNEL_6_GPIO_NUM

class esp_adc
{
private:
    static const adc_atten_t atten = ADC_ATTEN_DB_11;
    static const adc_unit_t unit = ADC_UNIT_1;
public:
    bool adc_init(void);
    int adc_median_filter(int old, int recent, int newest);
    void check_efuse(void); 
};


#endif