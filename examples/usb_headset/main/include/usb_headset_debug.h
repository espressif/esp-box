/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#pragma once

#if DEBUG_USB_HEADSET
#define bsp_codec_player_set_fs bsp_codec_player_set_fs_debug
#define bsp_codec_recorder_set_fs bsp_codec_recorder_set_fs_debug
#define bsp_codec_volume_set bsp_codec_volume_set_debug
#define bsp_codec_mute_set bsp_codec_mute_set_debug
#define bsp_i2s_read bsp_i2s_read_debug
#define bsp_i2s_write bsp_i2s_write_debug
#include "esp_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

void bsp_codec_player_set_fs_debug(uint32_t sample_rate, uint8_t resolution, uint8_t channel)
{
    TU_LOG1("bsp_codec_player_set_fs: sample_rate %" PRIu32 ", resolution %d, channel %d", sample_rate, resolution, channel);
}

void bsp_codec_recorder_set_fs_debug(uint32_t sample_rate, uint8_t resolution, uint8_t channel)
{
    TU_LOG1("bsp_codec_recorder_set_fs: sample_rate %" PRIu32 ", resolution %d, channel %d", sample_rate, resolution, channel);
}

void bsp_codec_volume_set_debug(int volume, int *volume_db)
{
    TU_LOG1("bsp_codec_volume_set: volume %d, volume_db %p", volume, volume_db);
}

void bsp_codec_mute_set_debug(int mute)
{
    TU_LOG1("bsp_codec_mute_set: mute %d", mute);
}

esp_err_t bsp_i2s_read_debug(void *buf, size_t size, size_t *bytes_read, TickType_t ticks_to_wait)
{
    static uint64_t last_time = 0;
    uint64_t current_time = esp_timer_get_time();
    size_t data_read_size = (current_time - last_time) / 1000 * DEFAULT_UAC_SAMPLE_RATE * DEFAULT_RECORDER_WIDTH / 8;
    if (data_read_size > size) {
        data_read_size = size;
    }

#if DEFAULT_RECORDER_CHANNEL == 1
    int16_t *data_buf = (int16_t *)buf;
    for (int i = 0; i < data_read_size / 2; i++) {
        data_buf[i] = (int16_t)(32767 * sin(2 * M_PI * 1000 * i / DEFAULT_UAC_SAMPLE_RATE));
    }
    *bytes_read = data_read_size;
#elif DEFAULT_RECORDER_CHANNEL == 2
    int16_t *data_buf = (int16_t *)buf;
    for (int i = 0; i < data_read_size / 4; i++) {
        data_buf[2 * i] = (int16_t)(32767 * sin(2 * M_PI * 1000 * i / DEFAULT_UAC_SAMPLE_RATE));
        data_buf[2 * i + 1] = (int16_t)(32767 * sin(2 * M_PI * 1000 * i / DEFAULT_UAC_SAMPLE_RATE));
    }
    *bytes_read = data_read_size;
#else
#error "Not support in debug mode"
#endif

    TU_LOG2("bsp_i2s_read: buf %p, size %d, bytes_read %d, ticks_to_wait %ld", buf, size, *bytes_read, ticks_to_wait);
    return ESP_OK;
}

esp_err_t bsp_i2s_write_debug(void *buf, size_t size, size_t *bytes_written, TickType_t ticks_to_wait)
{
    TU_LOG2("bsp_i2s_write: buf %p, size %d, bytes_written %d, ticks_to_wait %ld", buf, size, *bytes_written, ticks_to_wait);
    return ESP_OK;
}

#ifdef __cplusplus
}
#endif
#endif

#if DEBUG_SYSTEM_VIEW
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define SYSVIEW_SPK_WAIT_EVENT_ID     0
#define SYSVIEW_SPK_SEND_EVENT_ID     1
#define SYSVIEW_MIC_WAIT_EVENT_ID     2
#define SYSVIEW_MIC_READ_EVENT_ID     3

#define SYSVIEW_SPK_WAIT_EVENT_START()  SEGGER_SYSVIEW_OnUserStart(SYSVIEW_SPK_WAIT_EVENT_ID)
#define SYSVIEW_SPK_WAIT_EVENT_END()    SEGGER_SYSVIEW_OnUserStop(SYSVIEW_SPK_WAIT_EVENT_ID)
#define SYSVIEW_SPK_SEND_EVENT_START()  SEGGER_SYSVIEW_OnUserStart(SYSVIEW_SPK_SEND_EVENT_ID)
#define SYSVIEW_SPK_SEND_EVENT_END()    SEGGER_SYSVIEW_OnUserStop(SYSVIEW_SPK_SEND_EVENT_ID)
#define SYSVIEW_MIC_WAIT_EVENT_START()  SEGGER_SYSVIEW_OnUserStart(SYSVIEW_MIC_WAIT_EVENT_ID)
#define SYSVIEW_MIC_WAIT_EVENT_END()    SEGGER_SYSVIEW_OnUserStop(SYSVIEW_MIC_WAIT_EVENT_ID)
#define SYSVIEW_MIC_READ_EVENT_START()  SEGGER_SYSVIEW_OnUserStart(SYSVIEW_MIC_READ_EVENT_ID)
#define SYSVIEW_MIC_READ_EVENT_END()    SEGGER_SYSVIEW_OnUserStop(SYSVIEW_MIC_READ_EVENT_ID)
#else
#define SYSVIEW_SPK_WAIT_EVENT_START()
#define SYSVIEW_SPK_WAIT_EVENT_END()
#define SYSVIEW_SPK_SEND_EVENT_START()
#define SYSVIEW_SPK_SEND_EVENT_END()
#define SYSVIEW_MIC_WAIT_EVENT_START()
#define SYSVIEW_MIC_WAIT_EVENT_END()
#define SYSVIEW_MIC_READ_EVENT_START()
#define SYSVIEW_MIC_READ_EVENT_END()
#endif
