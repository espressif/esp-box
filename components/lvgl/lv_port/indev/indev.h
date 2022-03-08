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

typedef enum {
    BSP_INDEV_NONE = 0,
    BSP_INDEV_TP,
    BSP_INDEV_BTN,
    BSP_INDEV_HID,
    BSP_INDEV_MAX,
} indev_type_t; /* Type of input device */

typedef struct {
    int32_t x;
    int32_t y;
    int32_t z;
    int32_t btn_val;
    bool pressed;
} indev_data_t;


/**
 * @brief Call default input device init code
 * 
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t indev_init_default(void);

/**
 * @brief Get major indev data
 * 
 * @param data Pointer to `indev_data_t`
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t indev_get_major_value(indev_data_t *data);


#ifdef __cplusplus
}
#endif
