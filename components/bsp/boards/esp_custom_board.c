/**
 * @file esp_custom_board.c
 * @brief 
 * @version 0.1
 * @date 2021-10-20
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
#include "esp_custom_board.h"
#include "esp_err.h"

esp_err_t bsp_board_init(void)
{
    /* **************** ADD BOARD INITIAL CODE HERE **************** */

    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bsp_board_power_ctrl(power_module_t module, bool on)
{
    (void) module;
    (void) on;

    /* **************** ADD BOARD POWER CONTROL CODE HERE **************** */

    return ESP_ERR_NOT_SUPPORTED;
}
