/**
 * @file bsp_lcd.c
 * @brief 
 * @version 0.1
 * @date 2021-06-25
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

static const char *TAG = "bsp_lcd";

static esp_lcd_panel_io_handle_t io_handle = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;

static void *p_user_data = NULL;
static bool (*p_on_trans_done_cb)(void *) = NULL;
static SemaphoreHandle_t bsp_lcd_flush_done_sem = NULL;
static bool lcd_trans_done_cb(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t *, void *);

esp_err_t bsp_lcd_init(void)
{

    bsp_spi_lcd_init(&io_handle, lcd_trans_done_cb);

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = GPIO_LCD_RST,
        .color_space = ESP_LCD_COLOR_SPACE_BGR,
        .bits_per_pixel = 16,
    };

#if LCD_DISP_IC_NT
    ESP_ERROR_CHECK(esp_lcd_new_panel_nt35510(io_handle, &panel_config, &panel_handle));
#elif LCD_DISP_IC_ST
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
#else
    #error "Invalid LCD type"
#endif

    /**
     * @brief Configure LCD rotation and mirror
     * 
     */
    esp_err_t ret_val = ESP_OK;
    ret_val |= esp_lcd_panel_reset(panel_handle);
    ret_val |= esp_lcd_panel_init(panel_handle);
    ret_val |= esp_lcd_panel_invert_color(panel_handle, LCD_COLOR_INV);
    ret_val |= esp_lcd_panel_set_gap(panel_handle, 0, 0);
    ret_val |= esp_lcd_panel_swap_xy(panel_handle, LCD_SWAP_XY);
    ret_val |= esp_lcd_panel_mirror(panel_handle, LCD_MIRROR_X, LCD_MIRROR_Y);

    /**
     * @brief Configure LCD backlight IO.
     * 
     */
    if (GPIO_NUM_NC != GPIO_LCD_BL) {
        gpio_config_t bk_gpio_config = {
            .mode = GPIO_MODE_OUTPUT,
            /*!< Prevent left shift negtive value warning */
            .pin_bit_mask = GPIO_LCD_BL > 0 ? 1ULL << GPIO_LCD_BL : 0ULL,
        };
        gpio_config(&bk_gpio_config);
        gpio_set_level(GPIO_LCD_BL, GPIO_LCD_BL_ON);
    }

    /**
     * @brief Create mutex to receive LCD flush event.
     * 
     */
    if (NULL != bsp_lcd_flush_done_sem) {
        ESP_LOGE(TAG, "LCD already initialized");
        return ESP_ERR_INVALID_STATE;
    }

    bsp_lcd_flush_done_sem = xSemaphoreCreateBinary();

    if (NULL == bsp_lcd_flush_done_sem) {
        return ESP_ERR_NO_MEM;
    }

    /* If any function is checking LCD trans status before transmition */
    xSemaphoreGive(bsp_lcd_flush_done_sem);

    return ESP_OK;
}

esp_err_t bsp_lcd_deinit(void)
{
    esp_err_t ret_val = ESP_OK;

    ret_val |= esp_lcd_panel_del(panel_handle);
    ret_val |= esp_lcd_panel_io_del(io_handle);
    ret_val |= bsp_spi_lcd_deinit();

    return ret_val;
}

esp_err_t bsp_lcd_flush(int x1, int y1, int x2, int y2, const void *p_data, TickType_t ticks_to_wait)
{
    /* Wait for previous tansmition done */
    if (pdPASS != xSemaphoreTake(bsp_lcd_flush_done_sem, ticks_to_wait)) {
        return ESP_ERR_TIMEOUT;
    }

    return esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2, y2, p_data);
}

esp_err_t bsp_lcd_flush_wait_done(TickType_t ticks_to_wait)
{
    if (pdPASS != xSemaphoreTake(bsp_lcd_flush_done_sem, ticks_to_wait)) {
        return ESP_ERR_TIMEOUT;
    }

    xSemaphoreGive(bsp_lcd_flush_done_sem);

    return ESP_OK;
}

esp_err_t bsp_lcd_set_cb(bool (*trans_done_cb)(void *), void *data)
{
    if (esp_ptr_executable(trans_done_cb)) {
        p_on_trans_done_cb = trans_done_cb;
        p_user_data = data;
    } else {
        ESP_LOGE(TAG, "Invalid function pointer");
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

static bool lcd_trans_done_cb(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *user_data, void *event_data)
{
    (void) panel_io;
    (void) user_data;
    (void) event_data;

    /* Used for `bsp_lcd_flush_wait` */
    if (likely(NULL != bsp_lcd_flush_done_sem)) {
        xSemaphoreGive(bsp_lcd_flush_done_sem);
    }

    /* Call user registered function */
    if (NULL != p_on_trans_done_cb) {
        return p_on_trans_done_cb(p_user_data);
    }

    return false;
}
