/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include "esp_log.h"
#include "bsp_board.h"
#include "bsp_btn.h"
#include "indev.h"
#include "indev_hid.h"
#include "indev_tp.h"
#include "esp_err.h"

static indev_type_t g_major_dev = BSP_INDEV_NONE;

static esp_err_t indev_init(indev_type_t indev)
{
    switch (indev) {
    case BSP_INDEV_TP:
        return indev_tp_init();
        break;
    case BSP_INDEV_BTN:
        return ESP_OK; // button should be initialized before here
        break;
    case BSP_INDEV_HID:
        return indev_hid_init_default();
        break;
    default:
        return ESP_ERR_INVALID_ARG;
        break;
    }

    return ESP_ERR_INVALID_ARG;
}

esp_err_t indev_init_default(void)
{
    esp_err_t ret = ESP_ERR_NOT_SUPPORTED;
    const board_res_desc_t *brd = bsp_board_get_description();

    if (brd->BSP_INDEV_IS_TP) {
        ret = indev_init(BSP_INDEV_TP);
        if (ESP_OK == ret) {
            g_major_dev = BSP_INDEV_TP;
        }
    } else {
        ret = indev_init(BSP_INDEV_BTN);
        if (ESP_OK == ret) {
            g_major_dev = BSP_INDEV_BTN;
        }
    }

    return ret;
}

esp_err_t indev_get_major_value(indev_data_t *data)
{
    uint16_t x, y;
    uint8_t tp_num, btn_val;
    indev_hid_state_t hid_indev_data;

    switch (g_major_dev) {
    case BSP_INDEV_TP:
        if (ESP_OK == indev_tp_read(&tp_num, &x, &y, &btn_val)) {
            data->x = x;
            data->y = y;
            data->btn_val = btn_val;
            if (tp_num > 0) {
                data->pressed = true;
            } else {
                data->pressed = false;
            }
        }
        break;
    case BSP_INDEV_HID:
        if (ESP_OK == indev_hid_get_value(&hid_indev_data)) {
            data->x = hid_indev_data.x;
            data->y = hid_indev_data.y;
            data->btn_val = hid_indev_data.btn_val;
            if (data->btn_val) {
                data->pressed = true;
            } else {
                data->pressed = false;
            }
        }
        break;
    case BSP_INDEV_BTN:
        data->btn_val = bsp_btn_get_state(BOARD_BTN_ID_PREV) << 2 | bsp_btn_get_state(BOARD_BTN_ID_ENTER) << 1 | bsp_btn_get_state(BOARD_BTN_ID_NEXT);
        if (data->btn_val) {
            data->pressed = true;
        } else {
            data->pressed = false;
        }
        break;
    default:
        return ESP_ERR_INVALID_STATE;
    }

    return ESP_OK;
}

