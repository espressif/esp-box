// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include <string.h>
// #include "driver/gpio.h"
// #include "driver/adc.h"
// #include "esp_adc_cal.h"
// #include "button.h"

// #define NO_OF_SAMPLES   1
// #define DEFAULT_VREF    1100

// static esp_adc_cal_characteristics_t *adc_chars;
// static const adc_channel_t channel = ADC_CHANNEL_3;
// static const adc_atten_t atten = ADC_ATTEN_11db;
// static const adc_unit_t unit = ADC_UNIT_1;

// uint32_t voltage;

// void buttondetTask(void *arg)
// {
//     while (1) {
//         uint32_t adc_reading = 0;
//         for (int i = 0; i < NO_OF_SAMPLES; i++) {
//             adc_reading += adc1_get_raw((adc1_channel_t)channel);
//         }
//         adc_reading /= NO_OF_SAMPLES;
//         voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
//         vTaskDelay(100 / portTICK_PERIOD_MS);
//     }
// }

// void button_init()
// {
//     int min_vol = 1990;
//     int max_vol = 2040;
//     int last_trigger_time = 0;
//     int trigger_time = 0;
//     adc1_config_width(ADC_WIDTH_12Bit);
//     adc1_config_channel_atten(channel, atten);
//     //Characterize ADC
//     adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
//     esp_adc_cal_characterize(unit, atten, ADC_WIDTH_12Bit, DEFAULT_VREF, adc_chars);

//     xTaskCreatePinnedToCore(&buttondetTask, "buttondet", 2 * 1024, NULL, 8, NULL, 1);
// }

// void button_detect(char *s)
// {
//     uint32_t voltage1 = voltage;

// #ifdef CONFIG_ESP32_KORVO_V1_1_BOARD

//     if (voltage1 > 1970 && voltage1 < 2020)
//     {
//     	sprintf(s, "mode");
//     }
//     else if (voltage1 > 1640 && voltage1 < 1690)
//     {
//     	sprintf(s, "play");
//     }
//     else if (voltage1 > 1100 && voltage1 < 1150)
//     {
//     	sprintf(s, "set");
//     }
//     else if (voltage1 > 370 && voltage1 < 420)
//     {
//     	sprintf(s, "vol+");
//     }
//     else if (voltage1 > 800 && voltage1 < 850)
//     {
//     	sprintf(s, "vol-");
//     }
//     else if (voltage1 > 2400 && voltage1 < 2450)
//     {
//     	sprintf(s, "rec");    	
//     }
//     else
//     {
//     	sprintf(s, "null");
//     }

// #endif

// }