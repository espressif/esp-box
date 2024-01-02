/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <math.h>
#include "display.h"
#include "esp_dsp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "fft_convert.h"

#define RB_LENGTH N_SAMPLES * 12

static const char *TAG = "FFT_CONVERT";
static RingbufHandle_t rb_handle = {0};
static int N = N_SAMPLES;
static float *fft_buff;

static void fft_process(int16_t *data, float *out_buff)
{
    dsps_fft2r_sc16_ansi(data, N);
    dsps_bit_rev_sc16_ansi(data, N);
    for (int i = 0 ; i < (N_SAMPLES * 2) ; i++) {
        out_buff[i] = data[i];
        out_buff[i] = out_buff[i] / INT16_MAX;
    }

    for (int i = 0 ; i < N_SAMPLES; i++) {
        out_buff[i] = 10 * log10f(0.0000000000001 + out_buff[i * 2 + 0] * out_buff[i * 2 + 0] + out_buff[i * 2 + 1] * out_buff[i * 2 + 1]);
        out_buff[i] += 130;
    }
}

esp_err_t fft_init(void)
{
    fft_buff = (float *)calloc(1, N_SAMPLES * 2 * sizeof(float));

    assert(fft_buff != NULL);
    esp_err_t ret;
    ret = dsps_fft2r_init_sc16(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (ret  != ESP_OK) {
        ESP_LOGE(TAG, "Not possible to initialize FFT. Error = %i", ret);
        return ret;
    }
    return ESP_OK;
}

void rb_write(int16_t *buf, size_t size)
{
    if (buf == NULL) {
        return;
    }
    xRingbufferSend(rb_handle, (void *)buf, size, 0);
}

static void rb_init(void)
{
    rb_handle = xRingbufferCreate(RB_LENGTH, RINGBUF_TYPE_BYTEBUF);
    if (rb_handle == NULL) {
        ESP_LOGE(TAG, "Failed to create ring buffer");
    }
}

static void fft_convert_task(void *pvParameter)
{
    int16_t *data = NULL;
    size_t item_size = 0;
    size_t display_wtd = 0;
    /**
     * The total length of DMA buffer of I2S is:
     * `dma_frame_num * dma_desc_num * i2s_channel_num * i2s_data_bit_width / 8`.
     * Transmit `dma_frame_num * dma_desc_num` bytes to DMA is trade-off.
     */
    const size_t item_size_upto = N_SAMPLES * 2 * sizeof(int16_t);
    UBaseType_t items_waiting;

    while (1) {
        vRingbufferGetInfo(rb_handle, NULL, NULL, NULL, NULL, &items_waiting);
        if (items_waiting >= item_size_upto) {
            item_size = 0;
            /* receive data from ringbuffer and write it to I2S DMA transmit buffer */
            data = (int16_t *)xRingbufferReceiveUpTo(rb_handle, &item_size, (TickType_t)pdMS_TO_TICKS(20), item_size_upto);
            if (item_size > 0) {
                fft_process(data, fft_buff);
                vRingbufferReturnItem(rb_handle, (void *)data);
                display_draw(fft_buff);
                display_wtd = 0;
            }
        } else {
            if (display_wtd > 40) {
                display_draw(NULL);
                display_wtd = 0;
            }
            vTaskDelay((TickType_t)pdMS_TO_TICKS(1));
            display_wtd++;
        }
    }
}

esp_err_t fft_convert_init(void)
{
    rb_init();
    fft_init();
    xTaskCreate(fft_convert_task, "fft_convert_task", 1024 * 8, NULL, 1, NULL);
    return ESP_OK;
}
