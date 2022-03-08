/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Init touch panel
 * 
 * @return
 *    - ESP_OK: Success
 *    - ESP_ERR_NOT_SUPPORTED: Unsupported touch panel
 *    - Others: Fail
 */
esp_err_t indev_tp_init(void);

/**
 * @brief Read data from touch panel
 * 
 * @param tp_num Touch point number
 * @param x X coordinate
 * @param y Y coordinate
 * @param btn_val Button mask value
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t indev_tp_read(uint8_t *tp_num, uint16_t *x, uint16_t *y, uint8_t *btn_val);

#ifdef __cplusplus
}
#endif
