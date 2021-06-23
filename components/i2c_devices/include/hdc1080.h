/**
 * @file hdc1080.h
 * @brief HDC1080 driver header file.
 * @version 0.1
 * @date 2021-03-07
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

#include "bsp_i2c.h"
#include "i2c_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    hdc1080_temp_res_14bit = 0,
    hdc1080_temp_res_11bit,
    hdc1080_temp_res_max,
} hdc1080_temp_res_t;

typedef enum {
    hdc1080_humid_res_14bit = 0,
    hdc1080_humid_res_11bit,
    hdc1080_humid_res_8bit,
    hdc1080_humid_res_max,
} hdc1080_humid_res_t;

typedef enum {
	hdc1080_acq_mode_single = 0,
	hdc1080_acq_mode_both,
	hdc1080_acq_mode_max,
} hdc1080_acq_mode_t;

typedef union {
    struct {
        uint16_t humid_res:     2;  /* 00: 14 bit  01: 11 bit  10: 8bit, defined @hdc1080_humid_res_t */
        uint16_t temp_res:      1;  /* 0: 14 bit 1: 11 bit, defined @hdc1080_temp_res_t */
        uint16_t bat_status:    1;  /* 0: Battery voltage > 2.8V  1: Battery voltage < 2.8V  [Read only] */
        uint16_t acq_mode:      1;  /* 0: Temp or humid is acquired  1: Temp and humid are acquired in sequence, temp first */
        uint16_t heater:        1;  /* Set this bit to enable heater */
        uint16_t :              1;  /* Reserved, must be 0 */
        uint16_t soft_reset:    1;  /* Set this bit to reset HDC1080 */
        uint16_t :              8;  /* Reserved, must be 0 */
    };
    uint16_t val;
} hdc1080_config_reg_t;

typedef enum {
    hdc1080_measure_temp = 0x00,
    hdc1080_measure_humid = 0x01,
} hdc1080_measure_type_t;

/**
 * @brief 
 * 
 * @return esp_err_t 
 */
esp_err_t hdc1080_init(void);

/**
 * @brief 
 * 
 * @return esp_err_t 
 */
esp_err_t hdc1080_reset(void);

/**
 * @brief 
 * 
 * @return esp_err_t 
 */
esp_err_t hdc1080_set_mode(void);

/**
 * @brief 
 * 
 * @param measure_type 
 * @return esp_err_t 
 */
esp_err_t hdc1080_start_measure(hdc1080_measure_type_t measure_type);

/**
 * @brief 
 * 
 * @param data 
 * @return esp_err_t 
 */
esp_err_t hdc1080_get_measure_data(uint8_t *data);

#ifdef __cplusplus
}
#endif
