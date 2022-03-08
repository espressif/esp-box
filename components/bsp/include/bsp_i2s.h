/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_err.h"
#include "driver/i2s.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Init I2S bus
 * 
 * @param i2s_num I2S port num
 * @param sample_rate Audio sample rate. For I2S signal, it refers to LRCK/WS frequency
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_i2s_init(i2s_port_t i2s_num, uint32_t sample_rate);

/**
 * @brief Deinit I2S bus
 * 
 * @param i2s_num I2S port num
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_i2s_deinit(i2s_port_t i2s_num);

#ifdef __cplusplus
}
#endif
