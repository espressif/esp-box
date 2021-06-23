/**
 * @file esp_custom_board.c
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

#include "esp_err.h"
#include "bsp_board.h"

static const char *TAG = "board";

static bsp_board_t bsp_board = {
    .init = board_init,
    .pwr_ctrl = board_pwr_ctrl,
    .enter_low_power = board_enter_low_power,
}

esp_err_t board_init(void *data)
{
    (void) data;

    /*!< Place your own initialize code hera */
    ESP_LOGE(TAG, "Please replace with your own board initialize code.")

    return ESP_ERR_NOT_SUPPORTED;
}
