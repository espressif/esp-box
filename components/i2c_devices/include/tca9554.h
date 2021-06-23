/**
 * @file tca9554.h
 * @brief TCA9554 driver header.
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

/**
 * @brief 
 * 
 * @return esp_err_t 
 */
esp_err_t tca9554_init(void);

/**
 * @brief 
 * 
 * @param val 
 * @return esp_err_t 
 */
esp_err_t tca9554_set_configuration(uint8_t val);

/**
 * @brief 
 * 
 * @param pin_val 
 * @return esp_err_t 
 */
esp_err_t tca9554_write_output_pins(uint8_t pin_val);

/**
 * @brief 
 * 
 * @param pin_val 
 * @return esp_err_t 
 */
esp_err_t tca9554_read_output_pins(uint8_t *pin_val);

/**
 * @brief 
 * 
 * @param pin_val 
 * @return esp_err_t 
 */
esp_err_t tca9554_read_input_pins(uint8_t *pin_val);

#ifdef __cplusplus
}
#endif
