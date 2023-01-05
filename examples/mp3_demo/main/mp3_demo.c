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
#include "es8311.h"
#include "ui_audio.h"

static const char *TAG = "mp3_demo";

static i2s_chan_handle_t i2s_tx_chan;
static es8311_handle_t es8311_dev = NULL;

/* Buffer for reading/writing to I2S driver. Same length as SPIFFS buffer and I2S buffer, for optimal read/write performance.
   Recording audio data path:
   I2S peripheral -> I2S buffer (DMA) -> App buffer (RAM) -> SPIFFS buffer -> External SPI Flash.
   Vice versa for playback. */
#define BUFFER_SIZE     (1024)
#define SAMPLE_RATE     (44100)
#define DEFAULT_VOLUME  (60)

esp_err_t audio_codec_set_voice_volume(uint8_t volume)
{
    float v = volume;
    v *= 0.6f;
    v = -0.01f * (v * v) + 2.0f * v;

    return es8311_voice_volume_set(es8311_dev, volume, NULL);
}

static esp_err_t audio_mute_function(AUDIO_PLAYER_MUTE_SETTING setting) {
    // Volume saved when muting and restored when unmuting. Restoring volume is necessary
    // as es8311_set_voice_mute(true) results in voice volume (REG32) being set to zero.
    static uint8_t last_volume;

    uint8_t volume = get_sys_volume();
    if(volume != 0) {
        last_volume = volume;
    }
    
    ESP_RETURN_ON_ERROR(es8311_voice_mute(es8311_dev, setting == AUDIO_PLAYER_MUTE ? true : false), TAG, "set voice mute");

    // restore the voice volume upon unmuting
    if(setting == AUDIO_PLAYER_UNMUTE) {
        audio_codec_set_voice_volume(last_volume);
    }
    ESP_LOGI(TAG, "mute setting %d, volume:%d", setting, last_volume);

    return ESP_OK;
}

static esp_err_t audio_i2s_write(void * audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;
    ret = i2s_channel_write(i2s_tx_chan, (char *)audio_buffer, len, bytes_written, timeout_ms);
    return ret;
}

static esp_err_t audio_i2s_reconfig_clk(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    esp_err_t ret = ESP_OK;
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(rate),
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG((i2s_data_bit_width_t)bits_cfg, (i2s_slot_mode_t)ch),
        .gpio_cfg = BSP_I2S_GPIO_CFG,
    };

    ret |= i2s_channel_disable(i2s_tx_chan);
    ret |= i2s_channel_reconfig_std_clock(i2s_tx_chan, &std_cfg.clk_cfg);
    ret |= i2s_channel_reconfig_std_slot(i2s_tx_chan, &std_cfg.slot_cfg);
    ret |= i2s_channel_enable(i2s_tx_chan);
    return ret;
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

    /* Create and configure ES8311 I2C driver */
    es8311_dev = es8311_create(BSP_I2C_NUM, ES8311_ADDRRES_0);
    const es8311_clock_config_t clk_cfg = BSP_ES8311_SCLK_CONFIG(SAMPLE_RATE);
    es8311_init(es8311_dev, &clk_cfg, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16);
    es8311_voice_volume_set(es8311_dev, DEFAULT_VOLUME, NULL);

    /* Configure I2S peripheral and Power Amplifier */
    bsp_audio_init(NULL, &i2s_tx_chan, NULL);
    bsp_audio_poweramp_enable(true);

    audio_player_config_t config = { .mute_fn = audio_mute_function,
                                     .write_fn = audio_i2s_write,
                                     .clk_set_fn = audio_i2s_reconfig_clk,
                                     .priority = 1 };
    esp_err_t ret = audio_player_new(config);

    ui_audio_start(file_iterator);
}