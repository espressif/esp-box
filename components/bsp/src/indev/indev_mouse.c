/**
 * @file indev_mouse.c
 * @brief 
 * @version 0.1
 * @date 2021-08-25
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

#include <string.h>
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "indev_mouse";

static indev_mouse_satatus_data_t indev_mouse_satatus_data;
static indev_mouse_config_t indev_mouse_config = { .x_range = 65535, .y_range = 65535, };

esp_err_t indev_mouse_init(void)
{
    return ESP_OK;
}

esp_err_t indev_mouse_set_range(uint16_t x_range, uint16_t y_range)
{
    indev_mouse_config.x_range = x_range ? x_range : (uint16_t) 0xffff;
    indev_mouse_config.y_range = y_range ? y_range : (uint16_t) 0xffff;

    return ESP_OK;
}

esp_err_t indev_mouse_get_range(uint16_t *x_range, uint16_t *y_range)
{
    if (NULL == x_range || NULL == y_range) {
        return ESP_ERR_INVALID_ARG;
    }

    *x_range = indev_mouse_config.x_range;
    *y_range = indev_mouse_config.y_range;

    return ESP_OK;
}

esp_err_t indev_mouse_report_data(indev_mouse_report_data_t *data)
{
    if (NULL == data) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Expand data range to `int32_t` */
    int32_t x = indev_mouse_satatus_data.x;
    int32_t y = indev_mouse_satatus_data.y;
    int32_t z = indev_mouse_satatus_data.z;

    /* Add delta value to status data */
    x += data->dx;
    y += data->dy;
    z += data->dz;

    /* Check for coordinate range */
    x = x < 0 ? 0 : x;
    x = x > indev_mouse_config.x_range ? indev_mouse_config.x_range : x;
    y = y < 0 ? 0 : y;
    y = y > indev_mouse_config.y_range ? indev_mouse_config.y_range : y;

    /* Store value to `indev_mouse_satatus_data` */
    indev_mouse_satatus_data.x = x;
    indev_mouse_satatus_data.y = y;
    indev_mouse_satatus_data.z = z;

    return ESP_OK;
}

esp_err_t indev_mouse_get_status_data(indev_mouse_satatus_data_t *data)
{
    if (NULL == data) {
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(data, &indev_mouse_satatus_data, sizeof(indev_mouse_satatus_data));

    return ESP_OK;
}
