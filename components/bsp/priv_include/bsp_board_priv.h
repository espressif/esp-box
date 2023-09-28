/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief init sensor module
 *
 */
esp_err_t bsp_sensor_init(bsp_bottom_property_t *handle);

#ifdef __cplusplus
}
#endif
