/**
 * @file ltr303.h
 * @brief LTR303 driver header file.
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
    als_integration_time_100ms = 0,
    als_integration_time_50ms,
    als_integration_time_200ms,
    als_integration_time_400ms,
    als_integration_time_150ms,
    als_integration_time_250ms,
    als_integration_time_300ms,
    als_integration_time_350ms,
} als_integration_time_t;

typedef enum {
    als_measurement_rate_50ms = 0,
    als_measurement_rate_100ms,
    als_measurement_rate_200ms,
    als_measurement_rate_500ms,
    als_measurement_rate_1000ms,
    als_measurement_rate_2000ms,
} als_measurement_rate_t;

typedef enum {
	als_gain_1x = 0,
	als_gain_2x,
	als_gain_4x,
	als_gain_8x,
	als_gain_48x = 6,
	als_gain_96x,
} als_gain_t;

typedef enum {
    als_mode_standby = 0,
    als_mode_active,
} als_mode_t;
typedef struct {
    uint8_t reserve  : 3;
    uint8_t als_gain : 3;
    uint8_t sw_reset : 1;
    uint8_t als_mode : 1;
} als_contr_reg_t;

typedef struct {
	uint8_t reserve : 2;
	uint8_t als_integration_time : 3;
	uint8_t als_meas_report_rate : 3;
} als_meas_rate_reg_t;

#ifdef __cplusplus
}
#endif
