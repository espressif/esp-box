/**
 * @file esp32_s3_hmi_devkit.c
 * @brief 
 * @version 0.1
 * @date 2021-07-05
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

#include <stdbool.h>
#include "bsp_board.h"
#include "bsp_i2c.h"
#include "bsp_storage.h"
#include "bsp_tp.h"
#include "driver/gpio.h"
#include "esp_log.h"

esp_err_t bsp_board_init(void)
{
    /* Init I2C bus */
    ESP_ERROR_CHECK(bsp_i2c_init(I2C_NUM_0, 400 * 1000));

    return ESP_OK;
}
