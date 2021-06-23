/**
 * @file mpu6050.c
 * @brief MPU6050 driver.
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

#include "mpu6050.h"

static const char *TAG = "mpu6050";

#define WRITE_BIT  I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT   I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN   0x1     /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS  0x0     /*!< I2C master will not check ack from slave */
#define ACK_VAL    0x0         /*!< I2C ack value */
#define NACK_VAL   0x1         /*!< I2C nack value */

#define ALPHA 0.99             /*!< Weight for gyroscope */
#define RAD_TO_DEG (180.0f / 3.141592653589793f) /*!< Radians to degrees */

typedef struct {
    i2c_bus_device_handle_t i2c_dev;
    uint8_t dev_addr;
    uint32_t counter;
    float dt;  /*!< delay time between twice measurement, dt should be small (ms level) */
    struct timeval *timer;
} mpu6050_dev_t;

/* New codes start here */

static i2c_bus_device_handle_t mpu6050_handle = NULL;

static esp_err_t mpu6050_read_byte(uint8_t reg_addr, uint8_t *data)
{
    return i2c_bus_read_byte(mpu6050_handle, reg_addr, data);
}

static esp_err_t mpu6050_read_bytes(uint8_t reg_addr, size_t data_len, uint8_t *data)
{
    return i2c_bus_read_bytes(mpu6050_handle, reg_addr, data_len, data);
}

static esp_err_t mpu6050_write_byte(uint8_t reg_addr, uint8_t data)
{
    return i2c_bus_write_byte(mpu6050_handle, reg_addr, data);
}

static esp_err_t mpu6050_write_bytes(uint8_t reg_addr, size_t data_len, const uint8_t *data)
{
    return i2c_bus_write_bytes(mpu6050_handle, MPU6050_I2C_ADDRESS, data_len, data);
}

esp_err_t mpu6050_init(void)
{
    if (NULL != mpu6050_handle) {
        return ESP_FAIL;
    }

    bsp_i2c_add_device(&mpu6050_handle, MPU6050_I2C_ADDRESS);

    if (NULL == mpu6050_handle) {
        return ESP_FAIL;
    }

    mpu6050_wake_up();

    vTaskDelay(50);

    uint8_t mpu6050_deviceid;
    mpu6050_get_deviceid(&mpu6050_deviceid);
    ESP_LOGI(TAG, "mpu6050 device address is: 0x%02x\n", mpu6050_deviceid);
    mpu6050_set_acce_fs(ACCE_FS_4G);
    mpu6050_set_gyro_fs(GYRO_FS_500DPS);

    return ESP_OK;
}

esp_err_t mpu6050_get_deviceid(uint8_t *deviceid)
{
    return mpu6050_read_byte(MPU6050_WHO_AM_I, deviceid);
}

esp_err_t mpu6050_wake_up(void)
{
    uint8_t reg;
    esp_err_t ret;

    ret = mpu6050_read_byte(MPU6050_PWR_MGMT_1, &reg);

    if (ret != ESP_OK) {
        return ret;
    }

    reg &= (~BIT6);
    ret = mpu6050_write_byte(MPU6050_PWR_MGMT_1, reg);
    return ret;
}

esp_err_t mpu6050_sleep(void)
{
    esp_err_t ret;
    uint8_t reg;
    ret = mpu6050_read_byte(MPU6050_PWR_MGMT_1, &reg);

    if (ret != ESP_OK) {
        return ret;
    }

    reg |= BIT6;
    ret = mpu6050_write_byte(MPU6050_PWR_MGMT_1, reg);
    return ret;
}

esp_err_t mpu6050_set_acce_fs(mpu6050_acce_fs_t acce_fs)
{
    esp_err_t ret;
    uint8_t reg;
    ret = mpu6050_read_byte(MPU6050_ACCEL_CONFIG, &reg);

    if (ret != ESP_OK) {
        return ret;
    }

    reg &= (~BIT3);
    reg &= (~BIT4);
    reg |= (acce_fs << 3);
    ret = mpu6050_write_byte(MPU6050_ACCEL_CONFIG, reg);
    return ret;
}

esp_err_t mpu6050_set_gyro_fs(mpu6050_gyro_fs_t gyro_fs)
{
    esp_err_t ret;
    uint8_t reg;
    ret = mpu6050_read_byte(MPU6050_GYRO_CONFIG, &reg);

    if (ret != ESP_OK) {
        return ret;
    }

    reg &= (~BIT3);
    reg &= (~BIT4);
    reg |= (gyro_fs << 3);
    ret = mpu6050_write_byte(MPU6050_GYRO_CONFIG, reg);
    return ret;
}

esp_err_t mpu6050_get_acce_fs(mpu6050_acce_fs_t *acce_fs)
{
    esp_err_t ret;
    uint8_t reg;
    ret = mpu6050_read_byte(MPU6050_ACCEL_CONFIG, &reg);
    reg = (reg >> 3) & 0x03;
    *acce_fs = reg;
    return ret;
}

esp_err_t mpu6050_get_gyro_fs(mpu6050_gyro_fs_t *gyro_fs)
{
    esp_err_t ret;
    uint8_t reg;
    ret = mpu6050_read_byte( MPU6050_GYRO_CONFIG, &reg);
    reg = (reg >> 3) & 0x03;
    *gyro_fs = reg;
    return ret;
}

esp_err_t mpu6050_get_acce_sensitivity(float *acce_sensitivity)
{
    esp_err_t ret;
    uint8_t acce_fs;
    ret = mpu6050_read_byte(MPU6050_ACCEL_CONFIG, &acce_fs);
    acce_fs = (acce_fs >> 3) & 0x03;
    switch (acce_fs) {
        case ACCE_FS_2G:
            *acce_sensitivity = 16384;
            break;
        case ACCE_FS_4G:
            *acce_sensitivity = 8192;
            break;
        case ACCE_FS_8G:
            *acce_sensitivity = 4096;
            break;
        case ACCE_FS_16G:
            *acce_sensitivity = 2048;
            break;
        default:
            break;
    }
    return ret;
}

esp_err_t mpu6050_get_gyro_sensitivity(float *gyro_sensitivity)
{
    esp_err_t ret;
    uint8_t gyro_fs;
    ret = mpu6050_read_byte(MPU6050_ACCEL_CONFIG, &gyro_fs);
    gyro_fs = (gyro_fs >> 3) & 0x03;
    switch (gyro_fs) {
        case GYRO_FS_250DPS:
            *gyro_sensitivity = 131;
            break;
        case GYRO_FS_500DPS:
            *gyro_sensitivity = 65.5;
            break;
        case GYRO_FS_1000DPS:
            *gyro_sensitivity = 32.8;
            break;
        case GYRO_FS_2000DPS:
            *gyro_sensitivity = 16.4;
            break;
        default:
            break;
    }
    return ret;
}

esp_err_t mpu6050_get_raw_acce(mpu6050_raw_acce_value_t *raw_acce_value)
{
    uint8_t data_rd[6] = {0};
    esp_err_t ret = mpu6050_read_bytes(MPU6050_ACCEL_XOUT_H, 6, data_rd);
    raw_acce_value->raw_acce_x = (int16_t)((data_rd[0] << 8) + (data_rd[1]));
    raw_acce_value->raw_acce_y = (int16_t)((data_rd[2] << 8) + (data_rd[3]));
    raw_acce_value->raw_acce_z = (int16_t)((data_rd[4] << 8) + (data_rd[5]));
    return ret;
}

esp_err_t mpu6050_get_raw_gyro(mpu6050_raw_gyro_value_t *raw_gyro_value)
{
    uint8_t data_rd[6] = {0};
    esp_err_t ret = mpu6050_read_bytes(MPU6050_GYRO_XOUT_H, 6, data_rd);
    raw_gyro_value->raw_gyro_x = (int16_t)((data_rd[0] << 8) + (data_rd[1]));
    raw_gyro_value->raw_gyro_y = (int16_t)((data_rd[2] << 8) + (data_rd[3]));
    raw_gyro_value->raw_gyro_z = (int16_t)((data_rd[4] << 8) + (data_rd[5]));
    return ret;
}

esp_err_t mpu6050_get_acce(mpu6050_acce_value_t *acce_value)
{
    esp_err_t ret;
    float acce_sensitivity;
    mpu6050_raw_acce_value_t raw_acce;
    ret = mpu6050_get_acce_sensitivity(&acce_sensitivity);

    if (ret != ESP_OK) {
        return ret;
    }

    ret = mpu6050_get_raw_acce(&raw_acce);

    if (ret != ESP_OK) {
        return ret;
    }

    acce_value->acce_x = raw_acce.raw_acce_x / acce_sensitivity;
    acce_value->acce_y = raw_acce.raw_acce_y / acce_sensitivity;
    acce_value->acce_z = raw_acce.raw_acce_z / acce_sensitivity;
    return ESP_OK;
}

esp_err_t mpu6050_get_gyro(mpu6050_gyro_value_t *gyro_value)
{
    esp_err_t ret;
    float gyro_sensitivity;
    mpu6050_raw_gyro_value_t raw_gyro;
    ret = mpu6050_get_gyro_sensitivity(&gyro_sensitivity);

    if (ret != ESP_OK) {
        return ret;
    }

    ret = mpu6050_get_raw_gyro(&raw_gyro);

    if (ret != ESP_OK) {
        return ret;
    }

    gyro_value->gyro_x = raw_gyro.raw_gyro_x / gyro_sensitivity;
    gyro_value->gyro_y = raw_gyro.raw_gyro_y / gyro_sensitivity;
    gyro_value->gyro_z = raw_gyro.raw_gyro_z / gyro_sensitivity;
    return ESP_OK;
}

esp_err_t imu_mpu6050_acquire_gyro(float *gyro_x, float *gyro_y, float *gyro_z)
{
    mpu6050_gyro_value_t gyro = {0, 0, 0};

    if (gyro_x != NULL && gyro_y != NULL && gyro_z != NULL) {
        if (ESP_OK == mpu6050_get_gyro(&gyro)) {
            *gyro_x = gyro.gyro_x;
            *gyro_y = gyro.gyro_y;
            *gyro_z = gyro.gyro_z;
            return ESP_OK;
        }
    }

    *gyro_x = 0;
    *gyro_y = 0;
    *gyro_z = 0;
    return ESP_FAIL;
}

esp_err_t imu_mpu6050_acquire_acce(float *acce_x, float *acce_y, float *acce_z)
{
    mpu6050_acce_value_t acce = {0, 0, 0};

    if (acce_x != NULL && acce_y != NULL && acce_z != NULL) {
        if (ESP_OK == mpu6050_get_acce(&acce)) {
            *acce_x = acce.acce_x;
            *acce_y = acce.acce_y;
            *acce_z = acce.acce_z;
            return ESP_OK;
        }
    }

    *acce_x = 0;
    *acce_y = 0;
    *acce_z = 0;
    return ESP_FAIL;
}
