/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include "esp_log.h"
#include "bsp/esp-box-3.h"
#include "lvgl.h"
#include "ui/ui.h"
#include "app_ui_event.h"
#include "app_touch.h"

#define MAIN_TAG "ESP-BOX-JOYSTICK-MAIN"
#define APP_DISP_DEFAULT_BRIGHTNESS 80

void app_lvgl_display(void)
{
    bsp_display_lock(0);

    ui_init();

    bsp_display_unlock();
}

void app_main(void)
{
    event_state_group_init();

    flash_write_init();

    touch_sensor_init();

    touch_element_start();

    bsp_i2c_init();

    /* Initialize display and LVGL */
    bsp_display_start();

    /* Set default display brightness */
    bsp_display_brightness_set(APP_DISP_DEFAULT_BRIGHTNESS);

    /* Add and show objects on display */
    app_lvgl_display();

    ESP_LOGI(MAIN_TAG, "Example initialization done.");
}
