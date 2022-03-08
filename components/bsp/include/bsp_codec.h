/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "audio_hal.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CODEC_DEV_ES7210 = 0x01,
    CODEC_DEV_ES7243 = 0x02,
    CODEC_DEV_ES8156 = 0x04,
    CODEC_DEV_ES8311 = 0x08,
    // CODEC_DEV_ES8388,
} codec_dev_t;

/**
 * @brief Detect Codecs on the board
 * 
 * @param devices [out] bitmask of detected codec devices, see codec_dev_t
 * @return 
 *    - ESP_OK: Success
 *    - ESP_ERR_NOT_FOUND: Codec not detected on I2C bus
 *    - Others: Fail
 */
esp_err_t bsp_codec_detect(uint32_t *devices);

/**
 * @brief Initialize Codec on dev board
 * 
 * @param sample_rate 
 * @return 
 *    - ESP_OK: Success
 *    - ESP_ERR_NOT_FOUND: Codec not detected on I2C bus
 *    - ESP_ERR_NOT_SUPPORTED: Unsupported Codec or t
 *    - Others: Fail
 */
esp_err_t bsp_codec_init(audio_hal_iface_samples_t sample_rate);

/**
 * @brief Set output volume of Codec
 * 
 * @param volume volume to set
 * @return 
 *    - ESP_OK: Success
 *    - ESP_ERR_NOT_FOUND: Codec not detected on I2C bus
 */
esp_err_t bsp_codec_set_voice_volume(uint8_t volume);

/**
 * @brief Set input microphone gain of Codec
 * 
 * @param channel_mask mask of channel
 * @param volume volume to set
 * @return 
 *    - ESP_OK: Success
 *    - ESP_ERR_NOT_FOUND: Codec not detected on I2C bus
 */
esp_err_t bsp_codec_set_voice_gain(uint8_t channel_mask, uint8_t volume);

/**
 * @brief Configure I2S format
 *
 * @param fmt: I2S format
 * @return
 *     - ESP_OK: Success
 *     - ESP_FAIL: Fail
 */
esp_err_t bsp_codec_set_fmt(audio_hal_iface_format_t fmt);

#ifdef __cplusplus
}
#endif
