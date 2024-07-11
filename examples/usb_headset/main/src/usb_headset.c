/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <inttypes.h>
#include <math.h>
#include "esp_log.h"
#include "bsp/esp-bsp.h"
#include "bsp_board.h"
#include "fft_convert.h"
#include "usb_headset.h"
#include "usb_device_uac.h"

#include "tusb.h"
#include "usb_headset_debug.h"

const static char *TAG = "usb_headset";

static esp_err_t uac_device_output_cb(uint8_t *buf, size_t len, void *arg)
{
    size_t bytes_written = 0;
    esp_err_t ret = bsp_i2s_write(buf, len, &bytes_written, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2s write failed");
        return ESP_FAIL;
    }

#if !DEBUG_USB_HEADSET
    rb_write((int16_t *)buf, bytes_written);
#endif

    return ESP_OK;
}

static esp_err_t uac_device_input_cb(uint8_t *buf, size_t len, size_t *bytes_read, void *arg)
{
    esp_err_t ret = bsp_i2s_read(buf, len, bytes_read, 10);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2s read failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

static void uac_device_set_mute_cb(uint32_t mute, void *arg)
{
    bsp_codec_mute_set(mute);
    ESP_LOGI(TAG, "set uac-device mute to: %"PRIu32"", mute);
}

static void uac_device_set_volume_cb(uint32_t volume, void *arg)
{
    bsp_codec_volume_set(volume, NULL);
    ESP_LOGI(TAG, "set uac-device volume to: %"PRIu32"", volume);
}

esp_err_t usb_headset_init(void)
{
    uac_device_config_t config = {
        .output_cb = uac_device_output_cb,
        .input_cb = uac_device_input_cb,
        .set_mute_cb = uac_device_set_mute_cb,
        .set_volume_cb = uac_device_set_volume_cb,
        .cb_ctx = NULL,
    };
    esp_err_t ret = uac_device_init(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "uac device init failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "USB initialized");
    return ESP_OK;
}
