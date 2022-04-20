// Copyright 2020 Espressif Systems (Shanghai) Co. Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bsp_i2c.h"
#include "Message.h"
#include "inv_imu_driver.h"

static const char *TAG = "icm42670";


/*
 * Set of timers used throughout standalone applications
 */

#define SENSOR_ODR_1600HZ  1600
#define SENSOR_ODR_800HZ  800
#define SENSOR_ODR_400HZ   400
#define SENSOR_ODR_200HZ   200
#define SENSOR_ODR_100HZ   100
#define SENSOR_ODR_50HZ    50
#define SENSOR_ODR_25HZ    25
#define SENSOR_ODR_12_5HZ  12.5


static i2c_bus_device_handle_t g_i2c_dev_handle;

static int platform_io_hal_read_reg(void *context, uint8_t reg, uint8_t *buf, uint32_t len)
{
    return i2c_bus_read_bytes(g_i2c_dev_handle, reg, len, buf);
}

static int platform_io_hal_write_reg(void *context, uint8_t reg, const uint8_t *buf, uint32_t len)
{
    return i2c_bus_write_bytes(g_i2c_dev_handle, reg, len, buf);
}

static void platform_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static void platform_delay_us(uint32_t us)
{
    esp_rom_delay_us(us);
}

/*
 * Printer function for message facility
 */
static void msg_printer(int level, const char *str, va_list ap)
{
    static char out_str[2560]; /* static to limit stack usage */
    uint32_t idx = 0;
#define LOG_COLOR_BLACK   "30"
#define LOG_COLOR_RED     "31"
#define LOG_COLOR_GREEN   "32"
#define LOG_COLOR_BROWN   "33"
#define LOG_COLOR_BLUE    "34"
#define LOG_COLOR_PURPLE  "35"
#define LOG_COLOR_CYAN    "36"
#define LOG_COLOR(COLOR)  "\033[0;" COLOR "m"
#define LOG_BOLD(COLOR)   "\033[1;" COLOR "m"
#define LOG_RESET_COLOR   "\033[0m"
#define LOG_COLOR_E       LOG_COLOR(LOG_COLOR_RED)
#define LOG_COLOR_W       LOG_COLOR(LOG_COLOR_BROWN)
#define LOG_COLOR_I       LOG_COLOR(LOG_COLOR_GREEN)
    const char *s[INV_MSG_LEVEL_MAX] = {
        "",    // INV_MSG_LEVEL_OFF
        LOG_COLOR_E "[E]", // INV_MSG_LEVEL_ERROR
        LOG_COLOR_W "[W]", // INV_MSG_LEVEL_WARNING
        LOG_COLOR_I "[I]", // INV_MSG_LEVEL_INFO
        LOG_COLOR_BLUE "[V]", // INV_MSG_LEVEL_VERBOSE
        LOG_COLOR_BLUE "[D]", // INV_MSG_LEVEL_DEBUG
    };
    if (level >= INV_MSG_LEVEL_MAX) {
        printf("message error!\r\n");
        return;
    }

    idx += snprintf(&out_str[idx], sizeof(out_str) - idx, "%s %s: ", s[level], TAG);
    if (idx >= (sizeof(out_str))) {
        return;
    }
    idx += vsnprintf(&out_str[idx], sizeof(out_str) - idx, str, ap);
    if (idx >= (sizeof(out_str))) {
        return;
    }
    idx += snprintf(&out_str[idx], sizeof(out_str) - idx, LOG_RESET_COLOR "\r\n");
    if (idx >= (sizeof(out_str))) {
        return;
    }
    printf(out_str);
}

esp_err_t icm42670_init(void)
{
    esp_err_t ret = ESP_OK;
    if (NULL != g_i2c_dev_handle) {
        return ESP_FAIL;
    }

    bsp_i2c_add_device(&g_i2c_dev_handle, 0x68);

    if (NULL == g_i2c_dev_handle) {
        return ESP_FAIL;
    }

    /* Setup message facility to see internal traces from FW */
    inv_msg_setup(INV_MSG_LEVEL_INFO, msg_printer);

    inv_imu_set_serif(platform_io_hal_read_reg, platform_io_hal_write_reg);
    inv_imu_set_delay(platform_delay_ms, platform_delay_us);

    float hw_rate = 0.0;

    /* Init device */
    ret = inv_imu_initialize();

    if (ret != INV_ERROR_SUCCESS) {
        INV_MSG(INV_MSG_LEVEL_ERROR, "Failed to initialize INV concise driver!");
        return ret;
    }

    INV_MSG(INV_MSG_LEVEL_INFO, "IMU device successfully initialized");

    // We may customize full scale range here.
    ret |= inv_imu_set_accel_fsr(ACCEL_CONFIG0_FS_SEL_4g);
    ret |= inv_imu_set_gyro_fsr(GYRO_CONFIG0_FS_SEL_2000dps);

    // Below settings are required to configure and enable sensors.
    ret |= inv_imu_acc_enable();
    ret |= inv_imu_acc_set_rate(SENSOR_ODR_400HZ, 2, &hw_rate); //200Hz ODR, watermark: 2 packets.
    ret |= inv_imu_gyro_enable();
    ret |= inv_imu_gyro_set_rate(SENSOR_ODR_400HZ, 4, &hw_rate); //50Hz ODR, watermark 4.

    if (ret != 0) {
        INV_LOG(SENSOR_LOG_LEVEL, "Feature enable Failed. Do nothing %d", ret);
        return ret;
    }
    INV_LOG(SENSOR_LOG_LEVEL, "HW odr setting %f ", hw_rate);

    return ESP_OK;
}

esp_err_t icm42670_get_raw_data(AccDataPacket *accData,
                                GyroDataPacket *gyroData,
                                chip_temperature *imu_chip_temperature)
{
    int ret = inv_data_handler(accData, gyroData, imu_chip_temperature, 1);
    if (0 != ret) {
        return ESP_FAIL;
    }
    return ESP_OK;
}
