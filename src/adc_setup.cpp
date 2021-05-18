#include "adc_setup.h"

bool esp_adc::adc_init(void){
    adc1_config_width(ADC_WIDTH_BIT_10);
    adc1_config_channel_atten(battery_channel, atten);
    return true;
}

void esp_adc::check_efuse(void){
    static esp_adc_cal_characteristics_t *adc_chars;
    adc_chars = (esp_adc_cal_characteristics_t *)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, 1160, adc_chars);
    
    //Check TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        Serial.println("eFuse Two Point: Supported");
    } else {
        Serial.println("eFuse Two Point: NOT supported");
    }
    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        Serial.println("eFuse Vref: Supported");
    } else {
        Serial.println("eFuse Vref: NOT supported");
    }
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        Serial.println("Characterized using Two Point Value\n");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        Serial.println("Characterized using eFuse Vref");
    } else {
        Serial.println("Characterized using Default Vref");
    }
    switch (atten)
    {
        case 0:
        Serial.println("ADC_ATTEN_DB_0. No chages for the input voltage");
        break;
        case 1:
        Serial.println("ADC_ATTEN_DB_2_5. The input voltage will be reduce to about 1/1.34.");
        break;
        case 2:
        Serial.println("ADC_ATTEN_DB_6. The input voltage will be reduced to about 1/2");
        break;
        case 3:
        Serial.println("ADC_ATTEN_DB_11. The input voltage will be reduced to about 1/3.6");
        break;  
        default:
        Serial.println("Unknown attenuation.");
        break;
    }
    switch (adc_chars->bit_width)
    {
        case 0:
        Serial.println("ADC_WIDTH_BIT_9. ADC capture width is 9Bit");
        break;
        case 1:
        Serial.println("ADC_WIDTH_BIT_10. ADC capture width is 10Bit");
        break;
        case 2:
        Serial.println("ADC_WIDTH_BIT_11. ADC capture width is 11Bit");
        break;
        case 3:
        Serial.println("ADC_WIDTH_BIT_12. ADC capture width is 12Bit");  
        break;
        default:
        Serial.println("Unknown width.");
        break;
    }
}

int esp_adc::adc_median_filter(int old, int recent, int newest){
    int max_val = max(max(old, recent), newest);
    int min_val =min(min(old, recent), newest);
    int median = max_val ^ min_val ^ old ^ recent ^ newest;
    return median;
}