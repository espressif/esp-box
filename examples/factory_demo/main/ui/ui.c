/**
 * @file ui.c
 * @brief 
 * @version 0.1
 * @date 2021-10-29
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <time.h>
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include "ui_main.h"

static const char *TAG = "ui";

esp_err_t ui_main_start(void)
{
    if (NULL == lv_scr_act()) {
        ESP_LOGE(TAG, "LVGL not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    ui_mute_init();

    ui_sr_anim_init();

    boot_animate_start(lv_scr_act());

    ui_clock_update_start();

    return ESP_OK;
}

static void clock_blink_cb(lv_timer_t *timer)
{
    static time_t time_val;
    static bool disp = true;
    static char time_str[8];

    time(&time_val);
    struct tm time;
    localtime_r(&time_val, &time);

    disp = !disp;
    if (disp) {
        sprintf(time_str, "%02d:%02d", time.tm_hour, time.tm_min);
    } else {
        sprintf(time_str, "%02d %02d", time.tm_hour, time.tm_min);
    }

    ui_clock_set_time(time_str);
    ui_status_bar_set_time(time_str);
}

void ui_clock_update_start(void)
{
    static lv_timer_t *timer = NULL;
    if (NULL == timer) {
        timer = lv_timer_create(clock_blink_cb, 1000, NULL);
    }
}
