/**
 * @file indev_tp.c
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

#include <stdbool.h>
#include "esp_err.h"
#include "esp_log.h"
#include "bsp_i2c.h"

typedef struct {
    tp_type_t   dev_id;
    uint8_t     dev_addr;
    char *      dev_name;
} tp_prob_t;

typedef enum {
    TP_TYPE_FT = 0,
    TP_TYPE_GT,
    TP_TYPE_MAX,
} tp_type_t;

typedef struct {
    uint16_t x;
    uint16_t y;
    bool pressed;
    tp_gesture_t gesture;
} indev_tp_data_t;

static const tp_prob_t indev_tp_list[] = {
    { TP_TYPE_FT, 0x38, "FT" },     /*!< FocalTech */
    { TP_TYPE_GT, 0x14, "GT" },     /*!< Goodix */
    { TP_TYPE_MAX, 0xff, "MAX" },   /*!< Used for warning prevent */
};

typedef strcut {
    esp_err_t (*init)(void);                            /*!< Touch panel initialize */
    esp_err_t (*read)(uint16_t *, uint16_t *, bool *);  /*!< Read touch point(s) of touch panel */
    esp_err_t (*read_gesture)(uint8_t *);               /*!< Read gesture of touch panel */
} indev_tp_t;

static indev_tp_t indev_tp = NULL;

static const char *TAG = __FILE__;

static esp_err_t indev_tp_prob(void)
{
    size_t i = 0;
    for (; i < TP_TYPE_MAX; i++) {
        if (ESP_OK == bsp_i2c_probe_addr(indev_tp_list[i].dev_addr)) {
            ESP_LOGI(TAG, "Touch panel detected : %s", indev_tp_list[i].dev_name);
            break;
        }
    }

    if (TP_TYPE_MAX == i) {
        return ESP_ERR_NOT_FOUND;
    }

    /* Set indev_handle */

    return ESP_OK;
}

esp_err_t indev_tp_init(void)
{
    esp_err_t ret_val = indev_tp_prob();
    if (ESP_OK != ret_val) {
        ESP_LOGE(TAG, "Touch panel not found");
        return ret_val;
    }

    indev_tp.init();

    return ESP_OK;
}

typedef enum {
    TP_GESTURE_NONE = 0,
    TP_GESTURE_UP,
    TP_GESTURE_DOWN,
    TP_GESTURE_LEFT,
    TP_GESTURE_RIGHT,
    TP_GESTURE_ZOOM_IN,
    TP_GESTURE_ZOOM_OUT,
} tp_gesture_t;

esp_err_t indev_tp_read(indev_tp_data_t *data)
{
    uint16_t x, y;
    bool p;

    esp_err_t ret_val = indev_tp.read(&x, &y, &p);

    if (ESP_OK != ret_val) {
        data->x = 0;
        data->y = 0;
        data->pressed = false;
        data->gesture = TP_GESTURE_NONE;
        return ret_val;
    }

    data->x = x;
    data->y = y;
    data->pressed = p;

    /* Currently not ported */
    data->gesture = TP_GESTURE_NONE;

    return ESP_OK;
}

esp_err_t indev_tp_config_swap_reverse(bool swap, bool x_reverse, bool y_reverse)
{
    return ESP_OK;
}

esp_err_t indev_tp_read(indev_tp_data_t *data)
{
    return ESP_OK;
}
