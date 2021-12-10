/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_err.h"
#include "bsp_i2c.h"
#include "ft5x06.h"

/** @brief FT5x06 register map and function codes */
#define FT5x06_ADDR            (0x38)

#define FT5x06_DEVICE_MODE      (0x00)
#define FT5x06_GESTURE_ID       (0x01)
#define FT5x06_TOUCH_POINTS     (0x02)

#define FT5x06_TOUCH1_EV_FLAG   (0x03)
#define FT5x06_TOUCH1_XH        (0x03)
#define FT5x06_TOUCH1_XL        (0x04)
#define FT5x06_TOUCH1_YH        (0x05)
#define FT5x06_TOUCH1_YL        (0x06)

#define FT5x06_TOUCH2_EV_FLAG   (0x09)
#define FT5x06_TOUCH2_XH        (0x09)
#define FT5x06_TOUCH2_XL        (0x0A)
#define FT5x06_TOUCH2_YH        (0x0B)
#define FT5x06_TOUCH2_YL        (0x0C)

#define FT5x06_TOUCH3_EV_FLAG   (0x0F)
#define FT5x06_TOUCH3_XH        (0x0F)
#define FT5x06_TOUCH3_XL        (0x10)
#define FT5x06_TOUCH3_YH        (0x11)
#define FT5x06_TOUCH3_YL        (0x12)

#define FT5x06_TOUCH4_EV_FLAG   (0x15)
#define FT5x06_TOUCH4_XH        (0x15)
#define FT5x06_TOUCH4_XL        (0x16)
#define FT5x06_TOUCH4_YH        (0x17)
#define FT5x06_TOUCH4_YL        (0x18)

#define FT5x06_TOUCH5_EV_FLAG   (0x1B)
#define FT5x06_TOUCH5_XH        (0x1B)
#define FT5x06_TOUCH5_XL        (0x1C)
#define FT5x06_TOUCH5_YH        (0x1D)
#define FT5x06_TOUCH5_YL        (0x1E)

#define FT5x06_ID_G_THGROUP             (0x80)
#define FT5x06_ID_G_THPEAK              (0x81)
#define FT5x06_ID_G_THCAL               (0x82)
#define FT5x06_ID_G_THWATER             (0x83)
#define FT5x06_ID_G_THTEMP              (0x84)
#define FT5x06_ID_G_THDIFF              (0x85)
#define FT5x06_ID_G_CTRL                (0x86)
#define FT5x06_ID_G_TIME_ENTER_MONITOR  (0x87)
#define FT5x06_ID_G_PERIODACTIVE        (0x88)
#define FT5x06_ID_G_PERIODMONITOR       (0x89)
#define FT5x06_ID_G_AUTO_CLB_MODE       (0xA0)
#define FT5x06_ID_G_LIB_VERSION_H       (0xA1)
#define FT5x06_ID_G_LIB_VERSION_L       (0xA2)
#define FT5x06_ID_G_CIPHER              (0xA3)
#define FT5x06_ID_G_MODE                (0xA4)
#define FT5x06_ID_G_PMODE               (0xA5)
#define FT5x06_ID_G_FIRMID              (0xA6)
#define FT5x06_ID_G_STATE               (0xA7)
#define FT5x06_ID_G_FT5201ID            (0xA8)
#define FT5x06_ID_G_ERR                 (0xA9)

static i2c_bus_device_handle_t ft5x06_handle = NULL;

static inline esp_err_t ft5x06_read_byte(uint8_t reg_addr, uint8_t *data)
{
    return i2c_bus_read_byte(ft5x06_handle, reg_addr, data);
}

static inline esp_err_t ft5x06_read_bytes(uint8_t reg_addr, size_t data_len, uint8_t *data)
{
    return i2c_bus_read_bytes(ft5x06_handle, reg_addr, data_len, data);
}

static inline esp_err_t ft5x06_write_byte(uint8_t reg_addr, uint8_t data)
{
    return i2c_bus_write_byte(ft5x06_handle, reg_addr, data);
}

esp_err_t ft5x06_init(void)
{
    if (NULL != ft5x06_handle) {
        return ESP_ERR_INVALID_STATE;
    }

    bsp_i2c_add_device(&ft5x06_handle, FT5x06_ADDR);

    if (NULL == ft5x06_handle) {
        return ESP_FAIL;
    }

    esp_err_t ret_val = ESP_OK;

    // Valid touching detect threshold
    i2c_bus_write_byte(ft5x06_handle, FT5x06_ID_G_THGROUP, 70);

    // valid touching peak detect threshold
    i2c_bus_write_byte(ft5x06_handle, FT5x06_ID_G_THPEAK, 60);

    // Touch focus threshold
    i2c_bus_write_byte(ft5x06_handle, FT5x06_ID_G_THCAL, 16);

    // threshold when there is surface water
    i2c_bus_write_byte(ft5x06_handle, FT5x06_ID_G_THWATER, 60);

    // threshold of temperature compensation
    i2c_bus_write_byte(ft5x06_handle, FT5x06_ID_G_THTEMP, 10);

    // Touch difference threshold
    i2c_bus_write_byte(ft5x06_handle, FT5x06_ID_G_THDIFF, 20);

    // Delay to enter 'Monitor' status (s)
    i2c_bus_write_byte(ft5x06_handle, FT5x06_ID_G_TIME_ENTER_MONITOR, 2);

    // Period of 'Active' status (ms)
    i2c_bus_write_byte(ft5x06_handle, FT5x06_ID_G_PERIODACTIVE, 12);

    // Timer to enter 'idle' when in 'Monitor' (ms)
    i2c_bus_write_byte(ft5x06_handle, FT5x06_ID_G_PERIODMONITOR, 40);

    return ESP_OK;
}

static esp_err_t ft5x06_get_touch_points_num(uint8_t *touch_points_num)
{
    return ft5x06_read_byte(FT5x06_TOUCH_POINTS, touch_points_num);
}

esp_err_t ft5x06_read_pos(uint8_t *touch_points_num, uint16_t *x, uint16_t *y)
{
    esp_err_t ret_val = ESP_OK;
    static uint8_t data[4];

    ret_val |= ft5x06_get_touch_points_num(touch_points_num);

    if (0 == *touch_points_num) {
    } else {
        ret_val |= ft5x06_read_bytes(FT5x06_TOUCH1_XH, 4, data);

        *x = ((data[0] & 0x0f) << 8) + data[1];
        *y = ((data[2] & 0x0f) << 8) + data[3];
    }

    return ret_val;
}

esp_err_t fx5x06_read_gesture(ft5x06_gesture_t *gesture)
{
    return ft5x06_read_byte(FT5x06_GESTURE_ID, (uint8_t *)gesture);
}
