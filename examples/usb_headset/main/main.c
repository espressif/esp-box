/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "bsp_board.h"
#include "bsp/esp-bsp.h"

#include "display.h"
#include "fft_convert.h"
#include "usb_headset.h"

/* Can be used for i2s_std_gpio_config_t and/or i2s_std_config_t initialization */
#define BSP_I2S_GPIO_CFG       \
    {                          \
        .mclk = BSP_I2S_MCLK,  \
        .bclk = BSP_I2S_SCLK,  \
        .ws = BSP_I2S_LCLK,    \
        .dout = BSP_I2S_DOUT,  \
        .din = BSP_I2S_DSIN,   \
        .invert_flags = {      \
            .mclk_inv = false, \
            .bclk_inv = false, \
            .ws_inv = false,   \
        },                     \
    }

/* This configuration is used by default in bsp_audio_init() */
#define BSP_I2S_DUPLEX_MONO_CFG(_sample_rate)                                                         \
    {                                                                                                 \
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(_sample_rate),                                          \
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO), \
        .gpio_cfg = BSP_I2S_GPIO_CFG,                                                                 \
    }

void app_main(void)
{
    gpio_config_t io_conf = {0};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << GPIO_NUM_12);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(GPIO_NUM_12, 0);

#if !DEBUG_USB_HEADSET
    /* Initialize I2C (for touch and audio) */
    bsp_i2c_init();

    /* Initialize display */
    display_lcd_init();

    /* Init fft */
    fft_convert_init();

    /* Initialize audio i2s */
    i2s_std_config_t i2s_config = BSP_I2S_DUPLEX_MONO_CFG(DEFAULT_UAC_SAMPLE_RATE);
    i2s_config.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_384;
    bsp_audio_init(&i2s_config);

    /* Initialize bsp board */
    bsp_board_init();

    /* Initialize codec with defaults */
    bsp_codec_set_fs(DEFAULT_UAC_SAMPLE_RATE, DEFAULT_PLAYER_WIDTH, DEFAULT_PLAYER_CHANNEL);
    bsp_codec_volume_set(DEFAULT_VOLUME, NULL);
    bsp_codec_mute_set(false);
#endif

    usb_headset_init();
}
