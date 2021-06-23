/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <string.h>
#include "esp_log.h"
#include "es8311.h"
#include "board.h"

#define ES7243_ADDR         0x26

/*
* to define the clock soure of MCLK
*/
#define FROM_MCLK_PIN       0
#define FROM_SCLK_PIN       1
/*
* to define work mode(master or slave)
*/
#define MASTER_MODE         0
#define SLAVE_MODE          1
/*
* to define serial digital audio format
*/
#define I2S_FMT             0
#define LEFT_JUSTIFIED_FMT  1
#define DPS_PCM_A_FMT       2
#define DPS_PCM_B_FMT       3
/*
* to define resolution of PCM interface
*/
#define LENGTH_16BIT        0
#define LENGTH_24BIT        1
#define LENGTH_32BIT        2

static char *TAG = "DRV7243";

#define ES_ASSERT(a, format, b, ...) \
    if ((a) != 0) { \
        ESP_LOGE(TAG, format, ##__VA_ARGS__); \
        return b;\
    }

static int Es7243WriteReg(uint8_t regAdd, uint8_t data)
{
    int res = 0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    res |= i2c_master_start(cmd);
    res |= i2c_master_write_byte(cmd, ES7243_ADDR, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_write_byte(cmd, regAdd, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_write_byte(cmd, data, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_stop(cmd);
    res |= i2c_master_cmd_begin(0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    ES_ASSERT(res, "Es7243 Write Reg error", -1);
    return res;
}


int Es7243ReadReg(uint8_t regAdd)
{
    uint8_t data;
    int res = 0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    res |= i2c_master_start(cmd);
    res |= i2c_master_write_byte(cmd, ES7243_ADDR, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_write_byte(cmd, regAdd, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_stop(cmd);
    res |= i2c_master_cmd_begin(0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    cmd = i2c_cmd_link_create();
    res |= i2c_master_start(cmd);
    res |= i2c_master_write_byte(cmd, ES7243_ADDR | 0x01, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_read_byte(cmd, &data, 0x01 /*NACK_VAL*/);
    res |= i2c_master_stop(cmd);
    res |= i2c_master_cmd_begin(0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    ES_ASSERT(res, "Es7243 Read Reg error", -1);
    return (int)data;
}

esp_err_t Es7243Init(void)
{
    esp_err_t ret = ESP_OK;
    ret |= Es7243WriteReg(0x00, 0x01);
    ret |= Es7243WriteReg(0x06, 0x00);
    ret |= Es7243WriteReg(0x05, 0x1B);
    ret |= Es7243WriteReg(0x01, 0x0C);
    ret |= Es7243WriteReg(0x08, 0x43);
    ret |= Es7243WriteReg(0x05, 0x13);
    if (ret) {
        ESP_LOGE(TAG, "Es7243 initialize failed!");
        return ESP_FAIL;
    }
    return ret;
}
