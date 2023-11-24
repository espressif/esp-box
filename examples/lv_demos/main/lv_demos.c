/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "bsp/esp-bsp.h"

#include "demos/lv_demos.h"

void app_main(void)
{
    /* Initialize I2C (for touch and audio) */
    bsp_i2c_init();
    /* Initialize display and LVGL */
    bsp_display_start();

    /* Set display brightness to 100% */
    bsp_display_backlight_on();

    /**
     * @brief Demos provided by LVGL.
     *
     * @note Only enable one demo every time.
     *
     */
    bsp_display_lock(0);

#if LV_USE_DEMO_WIDGETS
    lv_demo_widgets();      /* A widgets example. This is what you get out of the box */
#endif

#if LV_USE_DEMO_KEYPAD_AND_ENCODER
    lv_demo_keypad_encoder();   /* Demonstrate the usage of encoder and keyboard */
#endif

#if LV_USE_DEMO_BENCHMARK
    lv_demo_benchmark();    /* A demo to measure the performance of LVGL or to compare different settings. */
#endif

#if LV_USE_DEMO_STRESS
    lv_demo_stress();       /* A stress test for LVGL. */
#endif

#if LV_USE_DEMO_MUSIC
    lv_demo_music();        /* A modern, smartphone-like music player demo. */
#endif

    bsp_display_unlock();
}
