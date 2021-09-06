/**
 * @file goodix.c
 * @brief 
 * @version 0.1
 * @date 2021-08-25
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

#include "bsp_i2c.h"
#include "driver/i2c.h"
#include "i2c_bus.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "goodix.h"

#define GOODIX_CHIP_ADDR_DEFAULT    0x14

/*!< WRITE ONLY REGISTER(S) */
#define GOODIX_REG_COMMAND		    0x8040
#define GOODIX_REG_ESD_CHECK    	0x8041
#define GOODIX_REG_PROXIMITY_EN     0x8042
#define GOODIX_REG_COMMAND_CHECK    0x8046

/*!< WRITEABLE/READABLE REGISTER(S) */
#define GOODIX_REG_CONFIG_VERSION	0x8047
#define GOODIX_REG_X_OUTPUT_MAX_L   0x8048
#define GOODIX_REG_X_OUTPUT_MAX_H   0x8049
#define GOODIX_REG_Y_OUTPUT_MAX_L   0x804A
#define GOODIX_REG_Y_OUTPUT_MAX_H   0x804B
#define GOODIX_REG_TOUCH_NUMBER     0x804C
#define GOODIX_REG_MODULE_SWITCH_1  0x804D
#define GOODIX_REG_MODULE_SWITCH_2  0x804E
#define GOODIX_REG_SHAKE_COUNT      0x804F

#define GOODIX_REG_GESTURE_SWITCH_1 0x8059
#define GOODIX_REG_GESTURE_SWITCH_2 0x805A

/*!< READ ONLY REGISTER(S) */
#define GOODIX_REG_PRODUCT_ID_1     0x8140
#define GOODIX_REG_PRODUCT_ID_2     0x8141
#define GOODIX_REG_PRODUCT_ID_3     0x8142
#define GOODIX_REG_PRODUCT_ID_4     0x8143
#define GOODIX_REG_FIRMWARE_VERSION_L   0x8144
#define GOODIX_REG_FIRMWARE_VERSION_H   0x8145
#define GOODIX_REG_X_RESOLUTION_L   0x8146
#define GOODIX_REG_X_RESOLUTION_H   0x8147
#define GOODIX_REG_Y_RESOLUTION_L   0x8148
#define GOODIX_REG_Y_RESOLUTION_H   0x8149
#define GOODIX_REG_VERNDOR_ID       0x814A

/* TOUCH STETE REGISTER(S) */
#define GOODIX_REG_TP_NUMBER        0x814E
#define GOODIX_REG_TP_NUMBER_MASK   0x07

#define GOODIX_REG_TP1_X_COOR_L     0x8158
#define GOODIX_REG_TP1_X_COOR_H     0x8159
#define GOODIX_REG_TP1_Y_COOR_L     0x815A
#define GOODIX_REG_TP1_Y_COOR_H     0x815B
#define GOODIX_REG_TP1_SIZE_L       0x815C
#define GOODIX_REG_TP1_SIZE_H       0x815D

#define GOODIX_REG_TP1_START        0x8158
#define GOODIX_REG_TP2_START        0x8160
#define GOODIX_REG_TP3_START        0x8168
#define GOODIX_REG_TP4_START        0x8178
#define GOODIX_REG_TP5_START        0x8178

#define GOODIX_REG_KEY_VALUE        0x817F

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t size;
} goodix_tp_reg_t;

static const char *TAG = "goodix";

static i2c_bus_device_handle_t goodix_handle = NULL;

static esp_err_t goodix_read(uint16_t reg_addr, uint8_t *data, size_t data_len)
{
    esp_err_t ret_val = ESP_OK;

    ret_val |= i2c_bus_read_reg16(goodix_handle, reg_addr, data_len, data);

    return ret_val;
}

static esp_err_t goodix_write(uint16_t reg_addr, uint8_t *data, size_t data_len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (GOODIX_CHIP_ADDR_DEFAULT << 1) | I2C_MASTER_WRITE, I2C_ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg_addr >> 8, I2C_ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg_addr, I2C_ACK_CHECK_EN);
    i2c_master_read(cmd, data, data_len, I2C_MASTER_ACK);
    i2c_master_stop(cmd);

    esp_err_t ret_val = i2c_bus_cmd_begin(goodix_handle, cmd);
    i2c_cmd_link_delete(cmd);

    return ret_val;
}


static esp_err_t goodix_chip_prob(void)
{
    uint16_t x_res = 0, y_res = 0;
    uint8_t id_str[5];
    id_str[4] = '\0';

    uint8_t cmd[2] = { 2, 0};

    esp_err_t ret_val = ESP_OK;

    ret_val |= goodix_read(GOODIX_REG_PRODUCT_ID_1, id_str, 4);
    ret_val |= goodix_read(GOODIX_REG_X_OUTPUT_MAX_L, &x_res, 2);
    ret_val |= goodix_read(GOODIX_REG_Y_OUTPUT_MAX_L, &y_res, 2);

    ESP_LOGI(TAG, "Goodix device detected : %s. Resolution : %04X * %04X", id_str, x_res, y_res);

    return ret_val;
}

esp_err_t goodix_tp_init(void)
{
    if (NULL != goodix_handle) {
        return ESP_FAIL;
    }

    esp_err_t ret_val = ESP_OK;

    ret_val |= bsp_i2c_add_device(&goodix_handle, GOODIX_CHIP_ADDR_DEFAULT);

    if (NULL == goodix_handle) {
        return ESP_FAIL;
    }

    ret_val |= goodix_chip_prob();

    return ESP_OK;
}

esp_err_t goodix_tp_read(uint8_t *tp_num, uint16_t *x, uint16_t *y)
{
    uint8_t tp_num_reg;
    goodix_tp_reg_t reg;
    esp_err_t ret_val = ESP_OK;

    ret_val |= goodix_read(GOODIX_REG_TP_NUMBER, &tp_num_reg, 1);
    ret_val |= goodix_read(GOODIX_REG_TP1_X_COOR_L, &reg, sizeof(goodix_tp_reg_t));

    *tp_num = tp_num_reg & 0b111;
    *x = (reg.x >> 8) + (reg.x << 8);
    *y = (reg.y >> 8) + (reg.y << 8);

    return ret_val;
}

esp_err_t goodix_tp_read_gesture(goodix_tp_gesture_t *gesture)
{
    esp_err_t ret_val = ESP_OK;

    uint16_t gesture_reg;

    ret_val |= goodix_read(GOODIX_REG_GESTURE_SWITCH_1, &gesture_reg, 2);

    *gesture = (goodix_tp_gesture_t) gesture_reg;

    return ret_val;
}
