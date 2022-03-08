/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include "esp_check.h"
#include "bsp_board.h"
#include "driver/adc.h"
#include "esp_err.h"
#include "button.h"

static const char *TAG = "bsp btn";

static button_handle_t *g_btn_handle = NULL;

static esp_err_t adc_button_init(button_handle_t *handle, size_t btn_id, adc1_channel_t adc_chanel, uint32_t voltage_err, const uint32_t voltage)
{
    esp_err_t ret_val = ESP_OK;
    button_config_t btn_cfg = {
        .type = BUTTON_TYPE_ADC,
        .adc_button_config = {
            .adc_channel = adc_chanel,
        },
    };

    btn_cfg.adc_button_config.button_index = btn_id;
    btn_cfg.adc_button_config.min = voltage - voltage_err;
    btn_cfg.adc_button_config.max = voltage + voltage_err;
    *handle = button_create(&btn_cfg);

    return ret_val;
}

static esp_err_t gpio_btn_init(button_handle_t *handle, const gpio_num_t gpio, const bool active_level)
{
    esp_err_t ret_val = ESP_OK;
    button_config_t btn_cfg = {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config = {
            .active_level = 0,
            .gpio_num = 0,
        },
    };

    btn_cfg.gpio_button_config.gpio_num = gpio;
    btn_cfg.gpio_button_config.active_level = active_level;
    *handle = button_create(&btn_cfg);

    return ret_val;
}

esp_err_t bsp_btn_init_default(void)
{
    ESP_RETURN_ON_FALSE(NULL == g_btn_handle, ESP_ERR_INVALID_STATE, TAG,  "button already initialized ");

    esp_err_t ret = ESP_OK;
    const board_res_desc_t *brd = bsp_board_get_description();
    g_btn_handle = calloc(sizeof(button_handle_t), brd->BUTTON_TAB_LEN);
    ESP_RETURN_ON_FALSE(NULL != g_btn_handle, ESP_ERR_NO_MEM, TAG,  "memory is insufficient for button");
    for (size_t i = 0; i < brd->BUTTON_TAB_LEN; i++) {
        const board_button_t *btn = &brd->BUTTON_TAB[i];
        if (GPIO_NUM_NC != btn->io_num) {
            ret = gpio_btn_init(&g_btn_handle[btn->id], btn->io_num, btn->active_level);
        } else {
            ret = adc_button_init(&g_btn_handle[btn->id], btn->id, brd->BUTTON_ADC_CHAN, 150, btn->vol);
        }
    }
    return ret;
}

esp_err_t bsp_btn_register_callback(board_btn_id_t btn_id, button_event_t event, button_cb_t callback, void *user_data)
{
    ESP_RETURN_ON_FALSE(NULL != g_btn_handle, ESP_ERR_INVALID_STATE, TAG,  "button not initialized");
    const board_res_desc_t *brd = bsp_board_get_description();
    ESP_RETURN_ON_FALSE(btn_id < brd->BUTTON_TAB_LEN, ESP_ERR_INVALID_ARG, TAG,  "button id incorrect");

    if (NULL == callback) {
        return button_unregister_cb(g_btn_handle[btn_id], event);
    }

    button_dev_t *btn = (button_dev_t *) g_btn_handle[btn_id];
    btn->cb_user_data = user_data;
    return button_register_cb(g_btn_handle[btn_id], event, callback);
}

esp_err_t bsp_btn_rm_all_callback(board_btn_id_t btn_id)
{
    ESP_RETURN_ON_FALSE(NULL != g_btn_handle, ESP_ERR_INVALID_STATE, TAG,  "button not initialized");
    const board_res_desc_t *brd = bsp_board_get_description();
    ESP_RETURN_ON_FALSE(btn_id < brd->BUTTON_TAB_LEN, ESP_ERR_INVALID_ARG, TAG,  "button id incorrect");

    for (size_t event = 0; event < BUTTON_EVENT_MAX; event++) {
        button_unregister_cb(g_btn_handle[btn_id], event);
    }
    return ESP_OK;
}

bool bsp_btn_get_state(board_btn_id_t btn_id)
{
    const board_res_desc_t *brd = bsp_board_get_description();
    ESP_RETURN_ON_FALSE(btn_id < brd->BUTTON_TAB_LEN, 0, TAG,  "button id incorrect");

    button_dev_t *btn = (button_dev_t *) g_btn_handle[btn_id];
    return btn->event == BUTTON_PRESS_DOWN ? 1 : 0;
}
