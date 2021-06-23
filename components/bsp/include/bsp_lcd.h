/**
 * @file bsp_lcd.h
 * @brief 
 * @version 0.1
 * @date 2021-07-05
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

#pragma once

#include <stdbool.h>
#include "bsp_board.h"
#include "esp_err.h"
#include "esp_lcd_types.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*bsp_lcd_trans_cb_t)(esp_lcd_panel_io_handle_t, void *, void *);

/**
 * @brief Initialize LCD
 * 
 * @return esp_err_t 
 */
esp_err_t bsp_lcd_init(void);

/**
 * @brief 
 * 
 * @return esp_err_t 
 */
esp_err_t bsp_lcd_deinit(void);

/**
 * @brief 
 * 
 * @param x1 
 * @param y1 
 * @param x2 
 * @param y2 
 * @param p_data 
 * @param ticks_to_wait 
 * @return esp_err_t 
 */
esp_err_t bsp_lcd_flush(int x1, int y1, int x2, int y2, const void *p_data, TickType_t ticks_to_wait);

/**
 * @brief 
 * 
 * @param ticks_to_wait 
 * @return esp_err_t 
 */
esp_err_t bsp_lcd_flush_wait_done(TickType_t ticks_to_wait);

/**
 * @brief 
 * 
 * @param trans_done_cb 
 * @param data 
 * @return esp_err_t 
 */
esp_err_t bsp_lcd_set_cb(bool (*trans_done_cb)(void *), void *data);

#if LCD_IFACE_SPI

/**
 * @brief 
 * 
 * @param p_io_handle 
 * @param trans_done_cb 
 * @return esp_err_t 
 */
esp_err_t bsp_spi_lcd_init(esp_lcd_panel_io_handle_t *p_io_handle, bsp_lcd_trans_cb_t trans_done_cb);

/**
 * @brief 
 * 
 * @return esp_err_t 
 */
esp_err_t bsp_spi_lcd_deinit(void);

#endif

#if LCD_IFACE_I80

/**
 * @brief 
 * 
 * @param p_io_handle 
 * @param trans_done_cb 
 * @return esp_err_t 
 */
esp_err_t bsp_i80_lcd_init(esp_lcd_panel_io_handle_t *p_io_handle, bsp_lcd_trans_cb_t trans_done_cb);

/**
 * @brief 
 * 
 * @return esp_err_t 
 */
esp_err_t bsp_i80_lcd_deinit(void);

#endif

#ifdef __cplusplus
}
#endif
