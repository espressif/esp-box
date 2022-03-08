/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bsp_i2c.h"
#include "bsp_board.h"
#include "indev_tp.h"
#include "esp_log.h"
#include "esp_err.h"

#include "ft5x06.h"
#include "tt21100.h"

static const char *TAG = "indev_tp";
typedef enum {
    TP_VENDOR_NONE = -1,
    TP_VENDOR_TT = 0,
    TP_VENDOR_FT,
    TP_VENDOR_MAX,
} tp_vendor_t;

typedef struct {
    char *dev_name;
    uint8_t dev_addr;
    tp_vendor_t dev_vendor;
} tp_dev_t;

static tp_dev_t tp_dev_list[] = {
    { "Parade Tech", 0x24, TP_VENDOR_TT },
    { "Focal Tech", 0x38, TP_VENDOR_FT },
};

static tp_vendor_t tp_vendor = TP_VENDOR_NONE;

static esp_err_t tp_prob(tp_vendor_t *p_tp_vendor)
{
    for (size_t i = 0; i < sizeof(tp_dev_list) / sizeof(tp_dev_list[0]); i++) {
        if (ESP_OK == bsp_i2c_probe_addr(tp_dev_list[i].dev_addr)) {
            *p_tp_vendor = (tp_vendor_t) i;
            ESP_LOGI(TAG, "Detected touch panel at 0x%02X. Vendor : %s",
                     tp_dev_list[i].dev_addr, tp_dev_list[i].dev_name);
            return ESP_OK;
        }
    }

    *p_tp_vendor = TP_VENDOR_NONE;
    ESP_LOGW(TAG, "Touch panel not detected");
    return ESP_ERR_NOT_FOUND;
}

esp_err_t indev_tp_init(void)
{
    esp_err_t ret_val = ESP_OK;

    ret_val |= tp_prob(&tp_vendor);

    switch (tp_vendor) {
    case TP_VENDOR_TT:
        ret_val |= tt21100_tp_init();
        break;
    case TP_VENDOR_FT:
        ret_val |= ft5x06_init();
        break;
    default:
        break;
    }

    return ret_val;
}

esp_err_t indev_tp_read(uint8_t *tp_num, uint16_t *x, uint16_t *y, uint8_t *btn_val)
{
    esp_err_t ret_val = ESP_OK;
    uint16_t btn_signal = 0;

    switch (tp_vendor) {
    case TP_VENDOR_TT:
        do {
            ret_val |= tt21100_tp_read();
        } while (tt21100_data_avaliable());

        ret_val |= tt21100_get_touch_point(tp_num, x, y);
        ret_val |= tt21100_get_btn_val(btn_val, &btn_signal);
        break;
    case TP_VENDOR_FT:
        ret_val |= ft5x06_read_pos(tp_num, x, y);
        break;
    default:
        return ESP_ERR_NOT_FOUND;
        break;
    }

    const board_res_desc_t *brd = bsp_board_get_description();
    if (brd->TOUCH_PANEL_SWAP_XY) {
        uint16_t swap = *x;
        *x = *y;
        *y = swap;
    }

    if (brd->TOUCH_PANEL_INVERSE_X) {
        *x = brd->LCD_WIDTH - ( *x + 1);
    }

    if (brd->TOUCH_PANEL_INVERSE_Y) {
        *y = brd->LCD_HEIGHT - (*y + 1);
    }

    ESP_LOGD(TAG, "[%3u, %3u]", *x, *y);

    return ret_val;
}
