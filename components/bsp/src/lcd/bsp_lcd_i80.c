/**
 * @file bsp_lcd_i80.c
 * @brief 
 * @version 0.1
 * @date 2021-08-20
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

#include "bsp_lcd.h"
#include "bsp_board.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#if LCD_IFACE_I80

static const char *TAG = "bsp_lcd_i80";

static esp_lcd_i80_bus_handle_t i80_bus = NULL;

esp_err_t bsp_i80_lcd_init(esp_lcd_panel_io_handle_t *p_io_handle, bsp_lcd_trans_cb_t trans_done_cb)
{
    esp_err_t ret_val = ESP_OK;

    if (NULL == p_io_handle) {
        ESP_LOGE(TAG, "Invalid LCD IO handle");
        return ESP_ERR_INVALID_ARG;
    }

    esp_lcd_i80_bus_config_t bus_config = {
        .dc_gpio_num = GPIO_LCD_DC,
        .wr_gpio_num = GPIO_LCD_WR,
        .data_gpio_nums = {
            GPIO_LCD_D00, GPIO_LCD_D01, GPIO_LCD_D02, GPIO_LCD_D03,
            GPIO_LCD_D04, GPIO_LCD_D05, GPIO_LCD_D06, GPIO_LCD_D07,
            GPIO_LCD_D08, GPIO_LCD_D09, GPIO_LCD_D10, GPIO_LCD_D11,
            GPIO_LCD_D12, GPIO_LCD_D13, GPIO_LCD_D14, GPIO_LCD_D15,
        },
        .bus_width = LCD_BUS_WIDTH,
        .max_transfer_bytes = LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t) + 32,
    };
    ret_val |= esp_lcd_new_i80_bus(&bus_config, &i80_bus);

    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = GPIO_LCD_CS,
        .pclk_hz = LCD_FREQ,
        .trans_queue_depth = 4,
        .dc_levels = {
            .dc_idle_level = 0,
            .dc_cmd_level = 0,
            .dc_dummy_level = 0,
            .dc_data_level = 1,
        },
        .flags = {
            .swap_color_bytes = 1,
        },
        .on_color_trans_done = trans_done_cb,
        .user_data = NULL,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ret_val |= esp_lcd_new_panel_io_i80(i80_bus, &io_config, p_io_handle);

    ESP_LOGD(TAG,
        "LCD Initialized with Intel - 8080 - %d Bit interface. Clock speed : %d",
        bus_config.bus_width, io_config.pclk_hz);

    return ret_val;
}

esp_err_t bsp_i80_lcd_deinit(void)
{
    esp_err_t ret_val = ESP_OK;

    ret_val |= esp_lcd_del_i80_bus(i80_bus);

    return ret_val;
}

#endif /* LCD_IFACE_I80 */
