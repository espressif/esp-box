/**
 * @file icm20602.c
 * @brief ICM20602 driver.
 * @version 0.1
 * @date 2021-03-07
 * 
 * @copyright Copyright 2021 Espressif Systems (Shanghai) Co. Ltd.
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "icm20602.h"

static i2c_bus_device_handle_t icm20602_handle = NULL;

static esp_err_t icm20602_read_byte(uint8_t reg_addr, uint8_t *data)
{
    return i2c_bus_read_byte(icm20602_handle, reg_addr, data);
}

static esp_err_t icm20602_read_bytes(uint8_t reg_addr, size_t data_len, uint8_t *data)
{
    return i2c_bus_read_bytes(icm20602_handle, reg_addr, data_len, data);
}

static esp_err_t icm20602_write_byte(uint8_t reg_addr, uint8_t data)
{
    return i2c_bus_write_byte(icm20602_handle, reg_addr, data);
}

static esp_err_t icm20602_write_bytes(uint8_t reg_addr, size_t data_len, uint8_t *data)
{
    return i2c_bus_write_bytes(icm20602_handle, reg_addr, data_len, data);
}

esp_err_t icm20602_init(void)
{
    if (NULL != icm20602_handle) {
        return ESP_FAIL;
    }

    bsp_i2c_add_device(&icm20602_handle, ICM20602_ADDR);

    if (NULL == icm20602_handle) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

/** ICM-20602 Functions */

esp_err_t icm20602_power_1_set(bool device_reset, bool device_sleep, bool cycle_en, bool standby_en, bool temp_dis, icm20602_clk_t clk_sel)
{
    icm20602_power_reg_1_t reg = {
        .device_reset = device_reset,
        .device_sleep = device_sleep,
        .cycle_en = cycle_en,
        .standby_en = standby_en,
        .temp_dis = temp_dis,
        .clk_sel = clk_sel,
    };

    uint8_t *data = (uint8_t *)&reg;

    return icm20602_write_byte(MPUREG_PWR_MGMT_1, *data);
}


esp_err_t icm20602_power_2_set(bool acc_x_disable, bool acc_y_disable, bool acc_z_disable, bool gyro_x_disable, bool gyro_y_disable, bool gyro_z_disable)
{
    icm20602_power_reg_2_t reg = {
        .reserve = 0,
        .stby_xa = acc_x_disable,
        .stby_ya = acc_y_disable,
        .stby_za = acc_z_disable,
        .stby_xg = gyro_x_disable,
        .stby_yg = gyro_y_disable,
        .stby_zg = gyro_z_disable,
    };

    uint8_t *data = (uint8_t *)&reg;

    return icm20602_write_byte(MPUREG_PWR_MGMT_2, *data);
}

esp_err_t icm20602_set_acc_range(icm20602_acc_range_t range)
{
    return ESP_OK;
}

esp_err_t icm20602_set_gyro_range(icm20602_gyro_range_t range)
{
    return ESP_OK;
}

// esp_err_t icm20602_set_acc_bandwidth(icm20602_acc_bw_t bandwidth)
// {
//     return ESP_OK;
// }

// esp_err_t icm20602_set_gyro_bandwidth(icm20602_gyro_bw_t bandwidth)
// {
//     return ESP_OK;
// }
