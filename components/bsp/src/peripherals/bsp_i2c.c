/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_log.h"
#include "bsp_i2c.h"
#include "bsp_board.h"
#include "driver/i2c.h"

static const char *TAG = "bsp_i2c";

#define ACK_CHECK_EN   0x1     /*!< I2C master will check ack from slave*/

static i2c_bus_handle_t i2c_bus_handle = NULL;

esp_err_t bsp_i2c_init(i2c_port_t i2c_num, uint32_t clk_speed, gpio_num_t scl_io, gpio_num_t sda_io)
{
    /* Check if bus is already created */
    if (NULL != i2c_bus_handle) {
        ESP_LOGE(TAG, "I2C bus already initialized.");
        return ESP_FAIL;
    }

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .scl_io_num = scl_io,
        .sda_io_num = sda_io,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = clk_speed,
    };

    i2c_bus_handle = i2c_bus_create(i2c_num, &conf);
    
    if (NULL == i2c_bus_handle) {
        ESP_LOGE(TAG, "Failed create I2C bus");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t bsp_i2c_deinit(void)
{
    /* Check if bus is already created */
    if (NULL == i2c_bus_handle) {
        ESP_LOGE(TAG, "I2C bus already deinitialized.");
        return ESP_FAIL;
    }

    i2c_bus_delete(&i2c_bus_handle);

    return ESP_OK;
}

esp_err_t bsp_i2c_add_device(i2c_bus_device_handle_t *i2c_device_handle, uint8_t dev_addr)
{
    if (NULL == i2c_bus_handle) {
        ESP_LOGE(TAG, "Failed create I2C device");
        return ESP_FAIL;
    }

    *i2c_device_handle = i2c_bus_device_create(i2c_bus_handle, dev_addr, 400000);

    if (NULL == i2c_device_handle) {
        ESP_LOGE(TAG, "Failed create I2C device");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t bsp_i2c_probe(void)
{
    if (NULL == bsp_i2c_bus_get_handle()) {
        ESP_LOGE(TAG, "I2C bus not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    i2c_bus_t *i2c_bus = (i2c_bus_t *) bsp_i2c_bus_get_handle();

    for (size_t i = 1; i < 0x80; i++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, ( i << 1 ), ACK_CHECK_EN);
        i2c_master_stop(cmd);
        esp_err_t ret_val = i2c_master_cmd_begin(i2c_bus->i2c_port, cmd, pdMS_TO_TICKS(500));
        i2c_cmd_link_delete(cmd);
        if(ESP_OK == ret_val) {
            ESP_LOGW(TAG, "Found I2C Device at 0x%02X", i);
        }
    }

    return ESP_OK;
}

esp_err_t bsp_i2c_probe_addr(uint8_t addr)
{
    /* Use 7 bit address here */
    if (addr >= 0x80) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Check if I2C bus initialized */
    if (NULL == bsp_i2c_bus_get_handle()) {
        ESP_LOGE(TAG, "I2C bus not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    /* Get I2C bus object from i2c_bus_handle */
    i2c_bus_t *i2c_bus = (i2c_bus_t *) bsp_i2c_bus_get_handle();

    /* Create probe cmd link */
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ( addr << 1 ), ACK_CHECK_EN);
    i2c_master_stop(cmd);

    /* Start probe cmd link */
    esp_err_t ret_val = i2c_master_cmd_begin(i2c_bus->i2c_port, cmd, pdMS_TO_TICKS(500));

    /* Delete cmd link after probe ends */
    i2c_cmd_link_delete(cmd);

    /* Get probe result if ESP_OK equals to ret_val */
    return ret_val;
}

i2c_bus_handle_t bsp_i2c_bus_get_handle(void)
{
    return i2c_bus_handle;
}
