/**
 * @file bsp_lcd_spi.c
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


#include "bsp_lcd.h"
#include "bsp_board.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_compiler.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "soc/soc_memory_layout.h"

static const char *TAG = "bsp_lcd_spi";

esp_err_t bsp_spi_lcd_init(esp_lcd_panel_io_handle_t *p_io_handle, bsp_lcd_trans_cb_t trans_done_cb)
{
    esp_err_t ret_val = ESP_OK;

    if (NULL == p_io_handle) {
        ESP_LOGE(TAG, "Invalid LCD IO handle");
        return ESP_ERR_INVALID_ARG;
    }

    spi_bus_config_t buscfg = {
        .sclk_io_num = GPIO_LCD_CLK,
#if (LCD_BUS_WIDTH == 1)
        .mosi_io_num = GPIO_LCD_DIN,
        .miso_io_num = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
#elif (LCD_BUS_WIDTH == 8)
        .data0_io_num = GPIO_LCD_D00,
        .data1_io_num = GPIO_LCD_D01,
        .data2_io_num = GPIO_LCD_D02,
        .data3_io_num = GPIO_LCD_D03,
        .data4_io_num = GPIO_LCD_D04,
        .data5_io_num = GPIO_LCD_D05,
        .data6_io_num = GPIO_LCD_D06,
        .data7_io_num = GPIO_LCD_D07,
        .flags = SPICOMMON_BUSFLAG_OCTAL,
#endif
        .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t)
    };


    ret_val |= spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = GPIO_LCD_DC,
        .cs_gpio_num = GPIO_LCD_CS,
        .pclk_hz = LCD_FREQ,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .lcd_cmd_bits =
#if LCD_DISP_IC_NT
        16,
#elif LCD_DISP_IC_ST
        8,
#else
        8,  /* TBD */
#endif
        .lcd_param_bits =
#if LCD_DISP_IC_NT
        16,
#elif LCD_DISP_IC_ST
        8,
#else
        8,  /* TBD */
#endif
        .on_color_trans_done = trans_done_cb,
        .user_ctx = NULL,
    };

#if (LCD_BUS_WIDTH == 8)
    io_config.flags.octal_mode = 1;
    io_config.spi_mode = 3;
#endif
    ret_val |= esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t) LCD_HOST, &io_config, p_io_handle);

    return ESP_OK;
}

esp_err_t bsp_spi_lcd_deinit(void)
{
    esp_err_t ret_val = ESP_OK;

    ret_val |= spi_bus_free(LCD_HOST);

    return ret_val;
}
