/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "audio_player.h"
#include "bsp/esp-bsp.h"
#include "esp_check.h"
#include "esp_log.h"
#include "file_iterator.h"
#include "ui_audio.h"
#include "bsp_board.h"

static const char *TAG = "mp3_demo";

static esp_err_t audio_mute_function(AUDIO_PLAYER_MUTE_SETTING setting)
{
    // Volume saved when muting and restored when unmuting. Restoring volume is necessary
    // as es8311_set_voice_mute(true) results in voice volume (REG32) being set to zero.
    static uint8_t last_volume;
    bsp_codec_config_t *codec_handle = bsp_board_get_codec_handle();

    uint8_t volume = get_sys_volume();
    if (volume != 0) {
        last_volume = volume;
    }

    codec_handle->mute_set_fn(setting == AUDIO_PLAYER_MUTE ? true : false);

    // restore the voice volume upon unmuting
    if (setting == AUDIO_PLAYER_UNMUTE) {
        codec_handle->volume_set_fn(volume, NULL);
    }
    ESP_LOGI(TAG, "mute setting %d, volume:%d", setting, last_volume);

    return ESP_OK;
}

void app_main(void)
{
    /* Initialize I2C (for touch and audio) */
    bsp_i2c_init();

    /* Initialize display and LVGL */
    bsp_display_start();

    /* Set display brightness to 100% */
    bsp_display_backlight_on();

    bsp_spiffs_mount();

    file_iterator_instance_t *file_iterator = file_iterator_new(BSP_MOUNT_POINT);
    assert(file_iterator != NULL);

    /* Configure I2S peripheral and Power Amplifier */
    bsp_audio_poweramp_enable(true);
    bsp_board_init();

    bsp_codec_config_t *codec_handle = bsp_board_get_codec_handle();
    audio_player_config_t config = { .mute_fn = audio_mute_function,
                                     .write_fn = codec_handle->i2s_write_fn,
                                     .clk_set_fn = codec_handle->i2s_reconfig_clk_fn,
                                     .priority = 1
                                   };
    ESP_ERROR_CHECK(audio_player_new(config));

    ui_audio_start(file_iterator);
}
