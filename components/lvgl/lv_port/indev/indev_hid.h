/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool init_ble_service;
} indev_hid_conf_t;

typedef struct {
    int32_t x;
    int32_t y;
    uint32_t btn_val;
    bool press;
} indev_hid_state_t;

/**
 * @brief Initialize HID indev
 * 
 * @param conf Config of indev
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t indev_hid_init(indev_hid_conf_t *conf);

/**
 * @brief Call default HID indev init code
 * 
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t indev_hid_init_default(void);

/**
 * @brief Get value of HID input device
 * 
 * @param state Pointer to bsp_hid_indev_state_t
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t indev_hid_get_value(indev_hid_state_t *state);

#ifdef __cplusplus
}
#endif
