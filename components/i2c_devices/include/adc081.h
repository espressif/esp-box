/**
 * @file adc081.h
 * @brief ADC081 driver header file.
 * @version 0.1
 * @date 2021-03-06
 * 
 * @copyright Copyright 2021 Espressif Systems (Shanghai) Co. Ltd.
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#pragma once

#include "esp_err.h"

#include "bsp_i2c.h"
#include "i2c_bus.h"

#define ADC081_DEFAULT_CONFIG   (0b11100000)

#ifdef __cplusplus
extern "C" {
#endif

typedef union {
    struct {
        uint8_t under_range :1;
        uint8_t over_range:1;
        uint8_t :6;
    };
    uint8_t val;
} adc081_alert_status_reg_t;

typedef enum {
    adc081_cycle_time_disable = 0,  /* Conversion off */
    adc081_cycle_time_32x,          /* 27   ksps */
    adc081_cycle_time_64x,          /* 13.5 ksps */
    adc081_cycle_time_128x,         /* 6.7  ksps */
    adc081_cycle_time_256x,         /* 3.4  ksps*/
    adc081_cycle_time_512x,         /* 1.7  ksps*/
    adc081_cycle_time_1024x,        /* 0.9  ksps*/
    adc081_cycle_time_2048x,        /* 0.4  ksps*/
} adc081_cycle_time_t;

typedef union {
    struct {
        uint8_t polarity :1;
        uint8_t :1;
        uint8_t alert_pin_en:1;
        uint8_t alert_flag_en:1;
        uint8_t alert_hold:1;
        uint8_t cycle_time:3;
    };
    uint8_t val;
} adc081_configuration_reg_t;

/**
 * @brief 
 * 
 * @return esp_err_t 
 */
esp_err_t adc081_init(void);

/**
 * @brief 
 * 
 * @param cfg 
 * @return esp_err_t 
 */
esp_err_t adc081_config(adc081_configuration_reg_t *cfg);

/**
 * @brief 
 * 
 * @return esp_err_t 
 */
esp_err_t adc081_config_default(void);

/**
 * @brief 
 * 
 * @param cfg 
 * @return esp_err_t 
 */
esp_err_t adc081_get_config(adc081_configuration_reg_t *cfg);

/**
 * @brief 
 * 
 * @param value 
 * @return esp_err_t 
 */
esp_err_t adc081_get_converted_value(uint8_t *value);



#ifdef __cplusplus
}
#endif
