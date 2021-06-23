/**
 * @file bsp_indev.c
 * @brief 
 * @version 0.1
 * @date 2021-08-24
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

#include "bsp_board.h"
#include "esp_err.h"

typedef enum {
    INDEV_MOUSE,
    INDEV_TOUCH,
    INDEV_KEYBOARD,
} indev_type_t;

typedef void *bsp_indev_handle_t;
typedef uint32_t bsp_indev_data_t;

esp_err_t bsp_indev_init(void)
{
    return ESP_OK;
}

esp_err_t bsp_indev_send_data(bsp_indev_handle_t handle, bsp_indev_data_t *data)
{
    return ESP_OK;
}

esp_err_t bsp_indev_recv_data(bsp_indev_handle_t handle, bsp_indev_data_t *data)
{
    //
}
