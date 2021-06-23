/**
 * @file bh1750.c
 * @brief BH1750 driver.
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

#include "bh1750.h"

#define WRITE_BIT  I2C_MASTER_WRITE  /*!< I2C master write */
#define READ_BIT   I2C_MASTER_READ   /*!< I2C master read */
#define ACK_CHECK_EN   0x1           /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS  0x0           /*!< I2C master will not check ack from slave */
#define ACK_VAL    0x0               /*!< I2C ack value */
#define NACK_VAL   0x1               /*!< I2C nack value */

#define BH_1750_MEASUREMENT_ACCURACY    1.2    /*!< the typical measurement accuracy of  BH1750 sensor */

#define BH1750_POWER_DOWN        0x00    /*!< Command to set Power Down*/
#define BH1750_POWER_ON          0x01    /*!< Command to set Power On*/
#define BH1750_DATA_REG_RESET    0x07    /*!< Command to reset data register, not acceptable in power down mode*/

static i2c_bus_device_handle_t bh1750_handle;

static esp_err_t bh1750_read_byte(uint8_t reg_addr, uint8_t *data)
{
    return i2c_bus_read_byte(bh1750_handle, reg_addr, data);
}

static esp_err_t bh1750_read_bytes(uint8_t reg_addr, size_t data_len, uint8_t *data)
{
    return i2c_bus_read_bytes(bh1750_handle, reg_addr, data_len, data);
}

static esp_err_t bh1750_write_byte(uint8_t reg_addr, uint8_t data)
{
    return i2c_bus_write_byte(bh1750_handle, reg_addr, data);
}

static esp_err_t bh1750_write_bytes(uint8_t reg_addr, size_t data_len, uint8_t *data)
{
    return i2c_bus_write_bytes(bh1750_handle, reg_addr, data_len, data);
}

esp_err_t bh1750_init(void)
{
    if (NULL != bh1750_handle) {
        return ESP_FAIL;
    }

    bsp_i2c_add_device(&bh1750_handle, BH1750_I2C_ADDRESS_DEFAULT);

    if (NULL == bh1750_handle) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

/***********************************************************/

esp_err_t bh1750_power_down(void)
{
    return bh1750_write_byte(NULL_I2C_MEM_ADDR, BH1750_POWER_DOWN);
}

esp_err_t bh1750_power_on(void)
{
    return bh1750_write_byte(NULL_I2C_MEM_ADDR, BH1750_POWER_ON);
}

esp_err_t bh1750_reset_data_register(void)
{
    bh1750_power_on();

    return bh1750_write_byte(NULL_I2C_MEM_ADDR, BH1750_DATA_REG_RESET);
}

esp_err_t bh1750_change_measure_time(uint8_t measure_time)
{
    uint8_t buf[2] = {0x40, 0x60};
    buf[0] |= measure_time >> 5;
    buf[1] |= measure_time & 0x1F;
    
    return bh1750_write_bytes(NULL_I2C_MEM_ADDR, 2, &buf[0]);
}

esp_err_t bh1750_set_measure_mode(bh1750_cmd_measure_t cmd_measure)
{
    return bh1750_write_byte(NULL_I2C_MEM_ADDR, cmd_measure);
}

esp_err_t bh1750_get_data(float *data)
{
    uint8_t bh1750_data[2] = {0};
    esp_err_t ret = bh1750_read_bytes(NULL_I2C_MEM_ADDR, 2, &bh1750_data[0]);
    *data = ((bh1750_data[0] << 8 | bh1750_data[1]) / BH_1750_MEASUREMENT_ACCURACY);
    return ret;
}

esp_err_t bh1750_get_light_intensity(bh1750_cmd_measure_t cmd_measure, float *data)
{
    esp_err_t ret = bh1750_set_measure_mode(cmd_measure);

    if (ret != ESP_OK) {
        return ret;
    }

    if ((cmd_measure == BH1750_CONTINUE_4LX_RES) || (cmd_measure == BH1750_ONETIME_4LX_RES)) {
        vTaskDelay(pdMS_TO_TICKS(30));
    } else {
        vTaskDelay(pdMS_TO_TICKS(180));
    }

    ret = bh1750_get_data(data);
    return ret;
}
