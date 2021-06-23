/**
 * @file ltr303.c
 * @brief LTR303 driver.
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

#include "ltr303.h"

#define LTR303_DEV_ADDR (0b0101001)

#define ALS_CONTR           (0x80)
#define ALS_MEAS_RATE       (0x85)
#define PART_ID             (0x86)
#define MANUFACT_ID         (0x87)
#define ALS_DATA_CH1_0      (0x88)
#define ALS_DATA_CH1_1      (0x89)
#define ALS_DATA_CH0_0      (0x8a)
#define ALS_DATA_CH0_1      (0x8b)
#define ALS_STATUS          (0x8c)
#define INTERRUPT_REG           (0x8f)
#define ALS_THRES_UP_0      (0x98)
#define ALS_THRES_UP_1      (0x99)
#define ALS_THRES_LOW_0     (0x99)
#define ALS_THRES_LOW_1     (0x9a)
#define INTERRUPT_PERSIST   (0x9e)

static i2c_bus_device_handle_t ltr303_handle = NULL;

static esp_err_t ltr303_read_byte(uint8_t reg_addr, uint8_t *data)
{
    return i2c_bus_read_byte(ltr303_handle, reg_addr, data);
}

static esp_err_t ltr303_read_bytes(uint8_t reg_addr, size_t data_len, uint8_t *data)
{
    return i2c_bus_read_bytes(ltr303_handle, reg_addr, data_len, data);
}

static esp_err_t ltr303_write_byte(uint8_t reg_addr, uint8_t data)
{
    return i2c_bus_write_byte(ltr303_handle, reg_addr, data);
}

esp_err_t ltr303_set_integration_time(als_integration_time_t time)
{
    uint8_t reg;
    ltr303_read_byte(ALS_MEAS_RATE, &reg);
    ((als_meas_rate_reg_t *) &reg)->als_integration_time = (uint8_t)time;
    return ltr303_write_byte(ALS_MEAS_RATE, reg);
}

esp_err_t ltr303_set_measurement_report_rate(als_measurement_rate_t rate)
{
    uint8_t reg;
    ltr303_read_byte(ALS_MEAS_RATE, &reg);
    ((als_meas_rate_reg_t *) &reg)->als_meas_report_rate = (uint8_t)rate;
    return ltr303_write_byte(ALS_MEAS_RATE, reg);
}

esp_err_t ltr303_soft_reset(void)
{
    /* No need to care about other bits */
    return ltr303_write_byte(ALS_CONTR, 0b10);
}

esp_err_t set_mode(als_mode_t mode)
{
    uint8_t reg;
    ltr303_read_byte(ALS_CONTR, &reg);
    ((als_contr_reg_t *) &reg)->als_mode = mode;

    return ltr303_write_byte(ALS_CONTR, reg);
}

esp_err_t ltr303_set_als_gain(als_gain_t gain)
{
    uint8_t reg;
    ltr303_read_byte(ALS_CONTR, &reg);
    ((als_contr_reg_t *) &reg)->als_gain = gain;
    return ltr303_write_byte(ALS_CONTR, reg);
}

esp_err_t ltr303_init(void)
{
    if (NULL != ltr303_handle) {
        return ESP_FAIL;
    }

    bsp_i2c_add_device(&ltr303_handle, LTR303_DEV_ADDR);

    if (NULL == ltr303_handle) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t ltr303_get_als_data(uint8_t *data)
{
    return ltr303_read_bytes(ALS_DATA_CH0_0, 4, data);
}

esp_err_t ltr303_get_als_status(bool *data_valid, als_gain_t *gain, bool *int_active, bool *is_new_data)
{
    typedef struct {
        uint8_t als_data_valid : 1;
        uint8_t als_data_gain_rate : 3;
        uint8_t als_interrupt_status : 1;
        uint8_t als_data_status : 1;
        uint8_t reserve : 2;
    } als_status_reg_t;

    uint8_t reg;
    ltr303_read_byte(ALS_STATUS, &reg);

    if (NULL != data_valid) {
        *data_valid = ((als_status_reg_t *) &reg)->als_data_valid;
    }

    if (NULL != gain) {
        *gain = ((als_status_reg_t *) &reg)->als_data_gain_rate;
    }

    if (NULL != int_active) {
        *int_active = ((als_status_reg_t *) &reg)->als_interrupt_status;
    }

    if (NULL != is_new_data) {
        *is_new_data = ((als_status_reg_t *) &reg)->als_data_status;
    }

    return ESP_OK;
}

esp_err_t ltr303_set_interrupt(bool int_polarity_high, bool int_en)
{
    typedef struct {
        uint8_t reserved_h : 5;
        uint8_t int_polarity : 1;
        uint8_t int_mode : 1;
        uint8_t reserved_l : 1;
    } interrupt_reg_t;

    uint8_t reg;

    ((interrupt_reg_t *) &reg)->int_polarity = int_polarity_high;
    ((interrupt_reg_t *) &reg)->int_mode = int_en;

    return ltr303_write_byte(INTERRUPT_REG, reg);
}
