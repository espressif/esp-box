/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bsp_lcd.h"
#include "bsp_board.h"
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


static esp_err_t bsp_spi_lcd_init(esp_lcd_panel_io_handle_t *p_io_handle, bsp_lcd_trans_cb_t trans_done_cb)
{
    esp_err_t ret_val = ESP_OK;
    const board_res_desc_t *brd = bsp_board_get_description();

    if (NULL == p_io_handle) {
        ESP_LOGE(TAG, "Invalid LCD IO handle");
        return ESP_ERR_INVALID_ARG;
    }

    if (brd->LCD_BUS_WIDTH == 8) {
        spi_bus_config_t buscfg = {
            .sclk_io_num = brd->GPIO_LCD_CLK,
            .data0_io_num = brd->GPIO_LCD_D00,
            .data1_io_num = brd->GPIO_LCD_D01,
            .data2_io_num = brd->GPIO_LCD_D02,
            .data3_io_num = brd->GPIO_LCD_D03,
            .data4_io_num = brd->GPIO_LCD_D04,
            .data5_io_num = brd->GPIO_LCD_D05,
            .data6_io_num = brd->GPIO_LCD_D06,
            .data7_io_num = brd->GPIO_LCD_D07,
            .flags = SPICOMMON_BUSFLAG_OCTAL,
            .max_transfer_sz = brd->LCD_WIDTH * brd->LCD_HEIGHT * sizeof(uint16_t)
        };
        ret_val |= spi_bus_initialize(brd->LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    } else if (brd->LCD_BUS_WIDTH == 1) {
        spi_bus_config_t buscfg = {
            .sclk_io_num = brd->GPIO_LCD_CLK,
            .mosi_io_num = brd->GPIO_LCD_DIN,
            .miso_io_num = GPIO_NUM_NC,
            .quadwp_io_num = GPIO_NUM_NC,
            .quadhd_io_num = GPIO_NUM_NC,
            .max_transfer_sz = brd->LCD_WIDTH * brd->LCD_HEIGHT * sizeof(uint16_t)
        };
        ret_val |= spi_bus_initialize(brd->LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    }
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = brd->GPIO_LCD_DC,
        .cs_gpio_num = brd->GPIO_LCD_CS,
        .pclk_hz = brd->LCD_FREQ,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = brd->LCD_CMD_BITS,
        .lcd_param_bits = brd->LCD_PARAM_BITS,
        .on_color_trans_done = trans_done_cb,
        .user_ctx = NULL,
    };

    if (brd->LCD_BUS_WIDTH == 8) {
        io_config.flags.octal_mode = 1;
        io_config.spi_mode = 3;
    }
    ret_val |= esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t) brd->LCD_HOST, &io_config, p_io_handle);

    return ESP_OK;
}

esp_err_t bsp_spi_lcd_deinit(void)
{
    esp_err_t ret_val = ESP_OK;
    const board_res_desc_t *brd = bsp_board_get_description();

    ret_val |= spi_bus_free(brd->LCD_HOST);

    return ret_val;
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

esp_err_t bsp_lcd_init(void)
{
    const board_res_desc_t *brd = bsp_board_get_description();
    bsp_spi_lcd_init(&io_handle, lcd_trans_done_cb);

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = brd->GPIO_LCD_RST,
        .color_space = brd->LCD_COLOR_SPACE,
        .bits_per_pixel = 16,
    };

    if (!brd->LCD_DISP_IC_ST) {
        ESP_ERROR_CHECK(esp_lcd_new_panel_nt35510(io_handle, &panel_config, &panel_handle));
    } else  {
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
    }

    /**
     * @brief Configure LCD rotation and mirror
     *
     */
    esp_err_t ret_val = ESP_OK;
    ret_val |= esp_lcd_panel_reset(panel_handle);
    ret_val |= esp_lcd_panel_init(panel_handle);
    ret_val |= esp_lcd_panel_invert_color(panel_handle, brd->LCD_COLOR_INV);
    ret_val |= esp_lcd_panel_set_gap(panel_handle, 0, 0);
    ret_val |= esp_lcd_panel_swap_xy(panel_handle, brd->LCD_SWAP_XY);
    ret_val |= esp_lcd_panel_mirror(panel_handle, brd->LCD_MIRROR_X, brd->LCD_MIRROR_Y);

    /**
     * @brief Configure LCD backlight IO.
     *
     */
    if (GPIO_NUM_NC != brd->GPIO_LCD_BL) {
        gpio_config_t bk_gpio_config = {
            .mode = GPIO_MODE_OUTPUT,
            /*!< Prevent left shift negtive value warning */
            .pin_bit_mask = brd->GPIO_LCD_BL > 0 ? 1ULL << brd->GPIO_LCD_BL : 0ULL,
        };
        gpio_config(&bk_gpio_config);
        gpio_set_level(brd->GPIO_LCD_BL, !brd->GPIO_LCD_BL_ON);
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

esp_err_t bsp_lcd_set_backlight(bool en)
{
    const board_res_desc_t *brd = bsp_board_get_description();
    return gpio_set_level(brd->GPIO_LCD_BL, en ? brd->GPIO_LCD_BL_ON : !brd->GPIO_LCD_BL_ON);
}
