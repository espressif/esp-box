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
#include "esp_lcd_panel_io.h"
#include "esp_lcd_types.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*bsp_lcd_trans_cb_t)(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t *, void *);

/**
 * @brief Init LCD
 * 
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_lcd_init(void);

/**
 * @brief Deinit LCD
 * 
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_lcd_deinit(void);

/**
 * @brief Flush LCD
 * 
 * @param x1 Start index on x-axis (x1 included)
 * @param y1 Start index on y-axis (y1 included)
 * @param x2 End index on x-axis (x2 not included)
 * @param y2 End index on y-axis (y2 not included)
 * @param p_data RGB color data that will be dumped to the specific range
 * @param ticks_to_wait Maximum blocking time
 * @return 
 *    - ESP_OK: Success
 *    - ESP_ERR_TIMEOUT: Timeout waiting for previous transaction finished
 *    - Others: Fail
 */
esp_err_t bsp_lcd_flush(int x1, int y1, int x2, int y2, const void *p_data, TickType_t ticks_to_wait);

/**
 * @brief Wait for a a single flush transaction finished
 * 
 * @param ticks_to_wait Maximum blocking time
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_lcd_flush_wait_done(TickType_t ticks_to_wait);

/**
 * @brief Set callback function when a single flush transaction is finished
 * 
 * @param trans_done_cb Callback function
 * @param data User data
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_lcd_set_cb(bool (*trans_done_cb)(void *), void *data);

/**
 * @brief Init LCD with SPI interface
 * 
 * @param p_io_handle LCD panel IO handle
 * @param trans_done_cb callback function when a single flush transaction is finished
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_spi_lcd_init(esp_lcd_panel_io_handle_t *p_io_handle, bsp_lcd_trans_cb_t trans_done_cb);

/**
 * @brief Deinit LCD with SPI interface
 * 
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_spi_lcd_deinit(void);

#ifdef __cplusplus
}
#endif
