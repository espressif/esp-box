/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <math.h>
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "display.h"
#include "driver/ledc.h"
#include "hal/spi_ll.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "fft_convert.h"

static const char *TAG = "display";
/****************** LCD Configuration ************************************************/
#define LCD_WIDTH              BSP_LCD_H_RES
#define LCD_HEIGHT             BSP_LCD_V_RES
#define LCD_BUFFER_SIZE        320*240*2

/****************** configure the example working mode *******************************/
#define BASIC_HIGH             40                          /* Subtract the height of the column height */
#define GROUP_WIDTH            10                          /* Width of each set of columns */
#define STRIP_WIDTH            8                           /* Width of the strip */
#define STRIP_DROP_SPEED       4                           /* The speed of the cube's fall */
#define COLOR_MODE             4                           /* 4: Customer mode 3: Tri-color gradient 2: Two-color gradient 1: One_color gradient */
#define SQUARE_MODE            2                           /* 1: Even fall mode 2: Gravity simulation mode */

#if SQUARE_MODE == 2
#define ACCELERATION           1.5
#define CRASH_SPEED_RATIO      0.05
#endif

#if COLOR_MODE == 1
#define COLOR_RANGE            320
#elif COLOR_MODE == 2
#define COLOR_RANGE            214
#elif COLOR_MODE == 3
#define COLOR_RANGE            107
#elif COLOR_MODE == 4
#define COLOR_RANGE            400
#endif

#define STRIP_NUM              320 / GROUP_WIDTH
#define INTERVAL_WIDTH         GROUP_WIDTH - STRIP_WIDTH

#ifndef SPI_LL_DATA_MAX_BIT_LEN
#define SPI_LL_DATA_MAX_BIT_LEN (1 << 18)
#define LCD_SPI_MAX_DATA_SIZE (SPI_LL_DATA_MAX_BIT_LEN / 8)
#else
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0)
#define LCD_SPI_MAX_DATA_SIZE (SPI_LL_DATA_MAX_BIT_LEN / 8)
#else
/**
 * @brief release5.0 align = 4092(4096 -4)
 */
#define LCD_SPI_MAX_DATA_SIZE ((SPI_LL_DATA_MAX_BIT_LEN / 8)/4092*4092)
#endif
#endif

static esp_lcd_panel_handle_t panel_handle = NULL;
static int16_t fre_point[STRIP_NUM] = {0};
static uint16_t *display_buffer = NULL;

typedef struct {
    float speed[STRIP_NUM];
    int16_t square_high[STRIP_NUM];
} display_square_t;

static display_square_t display_square = {0};

static void frequency_multiplier_calculation(void)
{
    double freq_interval = 24 * 1000 / N_SAMPLES;
    double x = STRIP_NUM / (log(24000) / log(2));
    double freq = 0;
    for (int i = 1; i < STRIP_NUM + 1; i++) {
        freq = pow(2, i / x) / freq_interval;
        if (freq > i) {
            if (freq > 1023) {
                fre_point[i - 1] = 1023;
            } else {
                fre_point[i - 1] = (int)freq;
            }
        } else {
            fre_point[i - 1] = i;
        }
        ESP_LOGD(TAG, "calculation fre %d", fre_point[i - 1]);
    }
}

static int adjust_height(int y, float coefficients)
{
    if (y <= 0) {
        return 0;
    }

    return pow(y, coefficients);
}

static uint16_t fade_color(float num, float range)
{
    int value = num / range;
    uint16_t r, g, b;
    switch (value) {
    case 0:
        r = (uint16_t)255 * num / range;
        g = 0;
        b = (uint16_t)255 * (1 - num / range);
        break;
    case 1:
        r = (uint16_t)255 * (1 - num / range);
        g = (uint16_t)255 * num / range;
        b = 0;
        break;
    default:
        r = 0;
        g = (uint16_t)255 * (1 - num / range);
        b = (uint16_t)255 * num / range;
        break;
    }
    return ((uint16_t)((r & 0xF8) | ((b & 0xF8) << 5) | ((g & 0x1c) << 11) | ((g & 0xE0) >> 5)));
}

static int draw_square(int y, int point_i)
{
#if SQUARE_MODE ==1
    if (y >= display_square.square_high[point_i]) {
        display_square.square_high[point_i] = y - 1;
    } else {
        if (display_square.square_high[point_i] <= 0) {
            display_square.square_high[point_i] = 1;
        } else {
            display_square.square_high[point_i] -= STRIP_DROP_SPEED;
        }
    }
#elif SQUARE_MODE == 2
    if (y >= display_square.square_high[point_i]) {
        display_square.square_high[point_i] = y - 1;
        display_square.speed[point_i] = CRASH_SPEED_RATIO * y;
    } else {
        if (display_square.square_high[point_i] <= 0) {
            display_square.square_high[point_i] = 1;
        } else {
            display_square.square_high[point_i] += display_square.speed[point_i];
            display_square.speed[point_i] -= ACCELERATION;
        }
    }
#endif
    return display_square.square_high[point_i];
}

esp_err_t display_draw(float *data)
{
    int fre_point_i = 0;
    uint16_t line_color = 0;
    uint16_t color = 0;
    int correct_y = 0;
    for (int x = 1; x < LCD_WIDTH; x += GROUP_WIDTH) {
        if (data != NULL) {
            correct_y = data[fre_point[fre_point_i]] - BASIC_HIGH;
            if ( x <= 40 ) {
                correct_y = adjust_height(correct_y, 1.15);
            } else if (x > 40 || x < 100) {
                correct_y = adjust_height(correct_y, 1.3);
            } else {
                correct_y = adjust_height(correct_y, 1);
            }
        }
        int square_high = LCD_HEIGHT - draw_square(correct_y, fre_point_i);
        correct_y = LCD_HEIGHT - correct_y;
        line_color = fade_color(x, COLOR_RANGE);
        for (int y = 0; y < LCD_HEIGHT; y++) {
            if (correct_y <= y) {
                color = line_color;
            } else if (square_high - STRIP_WIDTH <= y && square_high >= y) {
                color = 0xFFFF;
            } else {
                color = 0x0000;
            }

            for (int z = 0; z < STRIP_WIDTH; z++) {
                display_buffer[y * LCD_WIDTH + x + z] = color;
            }
        }
        fre_point_i++;
        correct_y = 0;
    }
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_WIDTH, LCD_HEIGHT, (void *)display_buffer);
    return ESP_OK;
}

esp_err_t display_lcd_init(void)
{
    esp_lcd_panel_io_handle_t io_handle = NULL;
    const bsp_display_config_t bsp_disp_cfg = {
        .max_transfer_sz = (LCD_SPI_MAX_DATA_SIZE),
    };
    bsp_display_new(&bsp_disp_cfg, &panel_handle, &io_handle);

    esp_lcd_panel_disp_on_off(panel_handle, true);
    bsp_display_backlight_on();

    for (int i = 0; i < STRIP_NUM; i++) {
        display_square.square_high[i] = 1;
        display_square.speed[i] = 0;
    }

    display_buffer = (uint16_t *)heap_caps_calloc(1, LCD_BUFFER_SIZE, MALLOC_CAP_INTERNAL);
    assert(display_buffer != NULL);
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_WIDTH, LCD_HEIGHT, (void *)display_buffer);
    frequency_multiplier_calculation();

    return ESP_OK;
}
