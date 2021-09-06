/**
 * @file tt21100.c
 * @brief 
 * @version 0.1
 * @date 2021-09-06
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "bsp_i2c.h"
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

static i2c_bus_device_handle_t tt21100_handle = NULL;

static esp_err_t tt21100_read(uint8_t *data, size_t data_len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (TT21100_CHIP_ADDR_DEFAULT << 1) | I2C_MASTER_READ, I2C_ACK_CHECK_EN);
    i2c_master_read(cmd, data, data_len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret_val = i2c_bus_cmd_begin(tt21100_handle, cmd);
    i2c_cmd_link_delete(cmd);

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

    return ret_val;
}

esp_err_t tt21100_tp_read(uint8_t *tp_num, uint16_t *x, uint16_t *y)
{
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
    } __attribute__((packed)) touch_report_struct_t;

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

    static uint16_t data_len;
    static uint8_t data[256];
    esp_err_t ret_val = ESP_OK;

    /* Get report data length */
    ret_val |= tt21100_read(&data_len, sizeof(data_len));

    /* Read report data if length */
    if (data_len < 0xff) {
        tt21100_read(data, data_len);

        touch_report_struct_t *p_report_data = (touch_report_struct_t *) data;
        touch_record_struct_t *p_record_data = (touch_record_struct_t *) &data[sizeof(touch_report_struct_t)];

        *x = p_record_data->x;
        *y = p_record_data->y;

        if (p_report_data->data_len > 2) {
            *tp_num = (data_len - sizeof(touch_report_struct_t)) / sizeof(touch_record_struct_t);
        } else {
            *tp_num = 0;
        }

        ESP_LOGD(TAG,
            "Data Length : %u\n"
            "Report ID : %u\n"
            "Time Stamp : %u\n"
            "Large Object : %u\n"
            "Record Num : %u\n"
            "Report Counter : %u\n"
            "Noise Effect : %u",
            p_report_data->data_len,
            p_report_data->report_id,
            p_report_data->time_stamp,
            p_report_data->large_object,
            p_report_data->record_num,
            p_report_data->report_counter,
            p_report_data->noise_efect);
        
        ESP_LOGD(TAG,
            "Touch Type : %u\n"
            "Tip : %u\n"
            "Event ID : %u\n"
            "Touch ID : %u\n"
            "X : %3u  Y : %3u\n"
            "Pressure : %u\n"
            "major_axis_length : %u\n"
            "Orientation : %u\n",
            p_record_data->touch_type,
            p_record_data->tip,
            p_record_data->event_id,
            p_record_data->touch_id,
            p_record_data->x,
            p_record_data->y,
            p_record_data->pressure,
            p_record_data->major_axis_length,
            p_record_data->orientation);
    }

    return ret_val;
}
