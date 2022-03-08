/**
 * @file lv_port.c
 * @brief
 * @version 0.1
 * @date 2021-06-28
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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "bsp_board.h"
#include "bsp_lcd.h"
#include "indev/indev.h"
#include "lv_port.h"
#include "lvgl.h"
#include "sdkconfig.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

static lv_disp_drv_t disp_drv;
static const char *TAG = "lv_port";
static bool lv_port_use_fixed_buffer = false;
static lv_indev_t *indev_touchpad;
static lv_indev_t *indev_button;
static uint8_t g_home_btn_val;

/**
 * @brief Task to generate ticks for LVGL.
 *
 * @param pvParam Not used.
 */
static void lv_tick_inc_cb(void *data)
{
    uint32_t tick_inc_period_ms = *((uint32_t *) data);

    lv_tick_inc(tick_inc_period_ms);
}

/**
 * @brief Tell LVGL that LCD flush done.
 *
 * @return true Call `portYIELD_FROM_ISR()` after esp-lcd ISR return.
 * @return false Do nothing after esp-lcd ISR return.v
 */
static bool lv_port_flush_ready(void)
{
    /* Inform the graphics library that you are ready with the flushing */
    lv_disp_flush_ready(&disp_drv);

    /* portYIELD_FROM_ISR (true) or not (false). */
    return false;
}

static void button_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    // static uint8_t prev_btn_id = 0;
    static uint32_t last_key = 0;

    /* Read touch point(s) via touch IC */
    indev_data_t indev_data;
    if (ESP_OK != indev_get_major_value(&indev_data)) {
        ESP_LOGE(TAG, "Failed read input device value");
        return;
    }

    /*Get the pressed button's ID*/
    if (indev_data.btn_val & 0x02) {
        last_key = LV_KEY_ENTER;
        data->state = LV_INDEV_STATE_PRESSED;
        ESP_LOGD(TAG, "ok");
    } else if (indev_data.btn_val & 0x04) {
        data->state = LV_INDEV_STATE_PRESSED;
        last_key = LV_KEY_PREV;
        ESP_LOGD(TAG, "prev");
    } else if (indev_data.btn_val & 0x01) {
        data->state = LV_INDEV_STATE_PRESSED;
        last_key = LV_KEY_NEXT;
        ESP_LOGD(TAG, "next");
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
    data->key = last_key;
}

/**
 * @brief Read touchpad data.
 *
 * @param indev_drv
 * @param data
 * @return IRAM_ATTR
 */
static IRAM_ATTR void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    uint8_t tp_num = 0;
    uint16_t x = 0, y = 0, btn_val = 0;
    /* Read touch point(s) via touch IC */
    indev_data_t indev_data;
    if (ESP_OK != indev_get_major_value(&indev_data)) {
        return;
    }

    ESP_LOGD(TAG, "Touch (%u) : [%3u, %3u] - 0x%02X", indev_data.pressed, indev_data.x, indev_data.y, indev_data.btn_val);

    /* FT series touch IC might return 0xff before first touch. */
    data->point.x = indev_data.x;
    data->point.y = indev_data.y;
    g_home_btn_val = indev_data.btn_val;

    if (indev_data.pressed) {
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

static void home_button_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    static uint8_t last_btn = 0;

    if (g_home_btn_val & 0x01) {
        data->state = LV_INDEV_STATE_PRESSED;
        last_btn = 0;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    /*Save the last pressed button's ID*/
    data->btn_id = last_btn;
}

/**
 * @brief LCD flush function callback for LVGL.
 *
 * @param disp_drv
 * @param area
 * @param color_p
 */
static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    (void) disp_drv;

    /*The most simple case (but also the slowest) to put all pixels to the screen one-by-one*/
    bsp_lcd_flush(area->x1, area->y1, area->x2 + 1, area->y2 + 1, (uint8_t *) color_p, portMAX_DELAY);
}

/**
 * @brief Initialize display driver for LVGL.
 *
 */
static void lv_port_disp_init(void)
{
    static lv_disp_draw_buf_t draw_buf_dsc;
    size_t disp_buf_height = 20;
    const board_res_desc_t *brd = bsp_board_get_description();

    /* Option 1 : Allocate memories from heap */
    uint32_t buf_alloc_caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
    lv_color_t *p_disp_buf = heap_caps_malloc(brd->LCD_WIDTH * disp_buf_height * sizeof(lv_color_t) * 2, buf_alloc_caps);
    lv_color_t *p_disp_buf1 = p_disp_buf;
    lv_color_t *p_disp_buf2 = p_disp_buf + brd->LCD_WIDTH * disp_buf_height;
    ESP_LOGI(TAG, "Try allocate two %u * %u display buffer, size:%u Byte", brd->LCD_WIDTH, disp_buf_height, brd->LCD_WIDTH * disp_buf_height * sizeof(lv_color_t) * 2);
    if (NULL == p_disp_buf) {
        ESP_LOGE(TAG, "No memory for LVGL display buffer");
        esp_system_abort("Memory allocation failed");
    }

    /* Option 2 : Using static space for display buffer */

    /* Initialize display buffer */
    lv_disp_draw_buf_init(&draw_buf_dsc, p_disp_buf1, p_disp_buf2, brd->LCD_WIDTH * disp_buf_height);

    /* Register the display in LVGL */
    lv_disp_drv_init(&disp_drv);

    /*Set the resolution of the display*/
    disp_drv.hor_res = brd->LCD_WIDTH;
    disp_drv.ver_res = brd->LCD_HEIGHT;

    /* Used to copy the buffer's content to the display */
    disp_drv.flush_cb = disp_flush;

    /*Set a display buffer*/
    disp_drv.draw_buf = &draw_buf_dsc;

    /**
     * @brief Fill a memory array with a color if you have GPU.
     * Note that, in lv_conf.h you can enable GPUs that has built-in support in LVGL.
     * But if you have a different GPU you can use with this callback.
     *
     */
    // disp_drv.gpu_fill_cb = gpu_fill;

    /* Use lcd_trans_done_cb to inform the graphics library that flush already done */
    bsp_lcd_set_cb(lv_port_flush_ready, NULL);

    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);
}

/**
 * @brief Initialize input device for LVGL.
 *
 * @return esp_err_t
 */
static esp_err_t lv_port_indev_init(void)
{
    /**
     * Here you will find example implementation of input devices supported by LittelvGL:
     *  - Touchpad
     *  - Mouse (with cursor support)
     *  - Keypad (supports GUI usage only with key)
     *  - Encoder (supports GUI usage only with: left, right, push)
     *  - Button (external buttons to press points on the screen)
     *
     *  The `..._read()` function are only examples.
     *  You should shape them according to your hardware
     */
    static lv_indev_drv_t indev_drv_tp;
    static lv_indev_drv_t indev_drv_btn;

    /* Initialize your touchpad if you have */
    const board_res_desc_t *brd = bsp_board_get_description();
    if (brd->BSP_INDEV_IS_TP) {
        ESP_LOGI(TAG, "Add TP input device to LVGL");
        lv_indev_drv_init(&indev_drv_tp);
        indev_drv_tp.type = LV_INDEV_TYPE_POINTER;
        indev_drv_tp.read_cb = touchpad_read;
        indev_touchpad = lv_indev_drv_register(&indev_drv_tp);
        if (brd->TOUCH_WITH_HOME_BUTTON) {
            ESP_LOGI(TAG, "Add HOME button input to LVGL");
            lv_indev_drv_init(&indev_drv_btn);
            indev_drv_btn.type = LV_INDEV_TYPE_BUTTON;
            indev_drv_btn.read_cb = home_button_read;
            indev_button = lv_indev_drv_register(&indev_drv_btn);
        }

    } else {
        ESP_LOGI(TAG, "Add KEYPAD input device to LVGL");
        lv_indev_drv_init(&indev_drv_btn);
        indev_drv_btn.type = LV_INDEV_TYPE_KEYPAD;
        indev_drv_btn.read_cb = button_read;
        indev_button = lv_indev_drv_register(&indev_drv_btn);
    }

#if CONFIG_LV_PORT_SHOW_MOUSE_CURSOR
    LV_IMG_DECLARE(mouse_cursor_icon)
    lv_obj_t *cursor_obj = lv_img_create(lv_scr_act());  /*Create an image object for the cursor */
    lv_img_set_src(cursor_obj, &mouse_cursor_icon);           /*Set the image source*/
    lv_indev_set_cursor(indev_touchpad, cursor_obj);             /*Connect the image  object to the driver*/
#endif
}

/**
 * @brief Create tick task for LVGL.
 *
 * @return esp_err_t
 */
static esp_err_t lv_port_tick_init(void)
{
    static const uint32_t tick_inc_period_ms = 5;
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = lv_tick_inc_cb,
        .name = "",     /* name is optional, but may help identify the timer when debugging */
        .arg = &tick_inc_period_ms,
        .dispatch_method = ESP_TIMER_TASK,
        .skip_unhandled_events = true,
    };

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));

    /* The timer has been created but is not running yet. Start the timer now */
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, tick_inc_period_ms * 1000));

    return ESP_OK;
}

esp_err_t lv_port_init(void)
{
    /* Initialize LVGL library */
    lv_init();

    /* Register display for LVGL */
    ESP_ERROR_CHECK(bsp_lcd_init());
    lv_port_disp_init();

    /* Register input device for LVGL*/
    ESP_ERROR_CHECK(indev_init_default());
    lv_port_indev_init();

    /* Initialize LVGL's tick source */
    lv_port_tick_init();

    /* Nothing error */
    return ESP_OK;
}
