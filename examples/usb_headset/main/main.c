/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "bsp_board.h"
#include "bsp/esp-bsp.h"

#include "display.h"
#include "fft_convert.h"
#include "usb_headset.h"

void app_main(void)
{
    /* Initialize I2C (for touch and audio) */
    bsp_i2c_init();

    /* Initialize display */
    display_lcd_init();

    /* Init fft */
    fft_convert_init();

    bsp_board_init();

    bsp_codec_set_fs(SAMPLE_RATE, WIDTH, CHANNEL);
    bsp_codec_volume_set(80, NULL);
    bsp_codec_mute_set(false);

    usb_headset_init();
}
