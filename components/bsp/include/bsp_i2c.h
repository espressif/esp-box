/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "driver/i2c.h"
#include "esp_err.h"
#include "i2c_bus.h"

#ifndef I2C_ACK_CHECK_EN
#define I2C_ACK_CHECK_EN    1
#endif

#ifndef I2C_ACK_CHECK_DIS
#define I2C_ACK_CHECK_DIS   0
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize I2C bus of dev board
 * 
 * @param i2c_num I2C port num
 * @param clk_speed I2C clock speed 
 * @param scl_io 
 * @param sda_io 
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_i2c_init(i2c_port_t i2c_num, uint32_t clk_speed, gpio_num_t scl_io, gpio_num_t sda_io);

/**
 * @brief Deinitialize I2C bus
 * 
 * @return esp_err_t 
 */
esp_err_t bsp_i2c_deinit(void);

/**
 * @brief Prob given address on I2C bus
 * 
 * @param addr 7 bit Address to prob
 * @return
 *    - ESP_OK: Device found on I2C bus
 *    - ESP_ERR_INVALID_ARG: Invalid I2C address
 *    - ESP_ERR_INVALID_STATE: I2C bus not initialized
 *    - Others: Device not found on I2C bus or error occurred during I2C transmission
 */
esp_err_t bsp_i2c_probe_addr(uint8_t addr);

/**
 * @brief Add device to I2C bus
 * 
 * @param i2c_device_handle Handle of I2C device
 * @param dev_addr 7 bit address of device
 * @return
 *    - ESP_OK Success
 *    - Others: Refer to error code `esp_err.h`.
 */
esp_err_t bsp_i2c_add_device(i2c_bus_device_handle_t *i2c_device_handle, uint8_t dev_addr);

/**
 * @brief Get handle of I2C bus
 * 
 * @return Handle of i2c bus. NULL if not or failed initialized.
 */
i2c_bus_handle_t bsp_i2c_bus_get_handle(void);

#ifdef __cplusplus
}
#endif
