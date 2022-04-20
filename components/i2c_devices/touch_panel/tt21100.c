/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bsp_i2c.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "i2c_bus.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/task.h"

#define TT21100_CHIP_ADDR_DEFAULT   (0x24)
#define TT21100_REG_TP_NUM          (0x1)
#define TT21100_REG_X_POS           (0x2)
#define TT21100_REG_Y_POS           (0x3)

static const char *TAG = "tt21100";

static uint8_t tp_num, btn_val;
static uint16_t x, y, btn_signal;
static i2c_bus_device_handle_t tt21100_handle = NULL;

static esp_err_t tt21100_read(uint8_t *data, size_t data_len)
{
    static uint8_t cmd_buffer[I2C_LINK_RECOMMENDED_SIZE(4)];
    i2c_cmd_handle_t cmd = i2c_cmd_link_create_static(cmd_buffer, I2C_LINK_RECOMMENDED_SIZE(4));

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (TT21100_CHIP_ADDR_DEFAULT << 1) | I2C_MASTER_READ, I2C_ACK_CHECK_EN);
    i2c_master_read(cmd, data, data_len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret_val = i2c_bus_cmd_begin(tt21100_handle, cmd);
    i2c_cmd_link_delete_static(cmd);

    return ret_val;
}

esp_err_t tt21100_tp_init(void)
{
    if (NULL != tt21100_handle) {
        return ESP_FAIL;
    }

    esp_err_t ret_val = ESP_OK;

    ret_val |= bsp_i2c_add_device(&tt21100_handle, TT21100_CHIP_ADDR_DEFAULT);

    if (NULL == tt21100_handle) {
        return ESP_FAIL;
    }

    uint16_t reg_val = 0;
    do {
        tt21100_read(&reg_val, sizeof(reg_val));
        vTaskDelay(pdMS_TO_TICKS(20));
    } while (0x0002 != reg_val);

    gpio_config_t io_conf_key;
    io_conf_key.intr_type = GPIO_INTR_DISABLE;
    io_conf_key.mode = GPIO_MODE_INPUT;
    io_conf_key.pin_bit_mask = 1ULL << GPIO_NUM_3;
    io_conf_key.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf_key.pull_up_en = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf_key));

    return ret_val;
}

esp_err_t tt21100_tp_read(void)
{
    typedef struct {
        uint8_t :5;
        uint8_t touch_type:3;
        uint8_t tip:1;
        uint8_t event_id:2;
        uint8_t touch_id:5;
        uint16_t x;
        uint16_t y;
        uint8_t pressure;
        uint16_t major_axis_length;
        uint8_t orientation;
    } __attribute__((packed)) touch_record_struct_t;

    typedef struct {
        uint16_t data_len;
        uint8_t report_id;
        uint16_t time_stamp;
        uint8_t :2;
        uint8_t large_object : 1;
        uint8_t record_num : 5;
        uint8_t report_counter:2;
        uint8_t :3;
        uint8_t noise_efect:3;
        touch_record_struct_t touch_record[0];
    } __attribute__((packed)) touch_report_struct_t;

    typedef struct {
        uint16_t length;        /*!< Always 14(0x000E) */
        uint8_t report_id;      /*!< Always 03h */
        uint16_t time_stamp;    /*!< Number in units of 100 us */
        uint8_t btn_val;        /*!< Only use bit[0..3] */
        uint16_t btn_signal[4];
    } __attribute__((packed)) button_record_struct_t;

    touch_report_struct_t *p_report_data = NULL;
    touch_record_struct_t *p_touch_data = NULL;
    button_record_struct_t *p_btn_data = NULL;

    static uint16_t data_len;
    static uint8_t data[256];
    esp_err_t ret_val = ESP_OK;

    /* Get report data length */
    ret_val |= tt21100_read(&data_len, sizeof(data_len));
    ESP_LOGD(TAG, "Data len : %u", data_len);

    /* Read report data if length */
    if (data_len < 0xff) {
        tt21100_read(data, data_len);
        switch (data_len) {
        case 2:     /* No avaliable data*/
            break;
        case 7:
        case 17:
        case 27:
            p_report_data = (touch_report_struct_t *) data;
            p_touch_data = &p_report_data->touch_record[0];
            x = p_touch_data->x;
            y = p_touch_data->y;

            tp_num = (data_len - sizeof(touch_report_struct_t)) / sizeof(touch_record_struct_t);
            for (size_t i = 0; i < tp_num; i++) {
                p_touch_data = &p_report_data->touch_record[i];
                ESP_LOGD(TAG, "(%zu) [%3u][%3u]", i, p_touch_data->x, p_touch_data->y);
            }
            break;
        case 14:    /* Button event */
            p_btn_data = (button_record_struct_t *) data;
            btn_val = p_btn_data->btn_val;
            btn_signal = p_btn_data->btn_signal[0];
            ESP_LOGD(TAG, "Len : %04Xh. ID : %02Xh. Time : %5u. Val : [%u] - [%04X][%04X][%04X][%04X]",
                p_btn_data->length, p_btn_data->report_id, p_btn_data->time_stamp, p_btn_data->btn_val,
                p_btn_data->btn_signal[0], p_btn_data->btn_signal[1], p_btn_data->btn_signal[2], p_btn_data->btn_signal[3]);
            break;
        default:
            break;
        }
    } else {
        return ESP_FAIL;
    }

    return ret_val;
}

esp_err_t tt21100_get_touch_point(uint8_t *p_tp_num, uint16_t *p_x, uint16_t *p_y)
{
    *p_x = x;
    *p_y = y;
    *p_tp_num = tp_num;

    return ESP_OK;
}

esp_err_t tt21100_get_btn_val(uint8_t *p_btn_val, uint16_t *p_btn_signal)
{
    *p_btn_val = btn_val;
    *p_btn_signal = btn_signal;

    return ESP_OK;
}

bool tt21100_data_avaliable(void)
{
    int level = gpio_get_level(GPIO_NUM_3);
    if (level) {
        return false;
    }

    return true;
}
