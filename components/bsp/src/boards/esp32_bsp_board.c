/*
 * SPDX-FileCopyrightText: 2015-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>

#include "esp_log.h"
#include "esp_check.h"

#include "bsp/esp-bsp.h"
#include "bsp_board.h"
#include "bsp_board_priv.h"

#define CODEC_DEFAULT_SAMPLE_RATE          (16000)
#define CODEC_DEFAULT_BIT_WIDTH            (16)
#define CODEC_DEFAULT_ADC_VOLUME           (24.0)
#define CODEC_DEFAULT_CHANNEL              (2)

static const pmod_pins_t g_pmod[2] = {
    {
        {BSP_PMOD2_IO5, BSP_PMOD2_IO6, BSP_PMOD2_IO7, BSP_PMOD2_IO8},
        {BSP_PMOD2_IO1, BSP_PMOD2_IO2, BSP_PMOD2_IO3, BSP_PMOD2_IO4},
    },
    {
        {BSP_PMOD1_IO5, BSP_PMOD1_IO6, BSP_PMOD1_IO7, BSP_PMOD1_IO8},
        {BSP_PMOD1_IO1, BSP_PMOD1_IO2, BSP_PMOD1_IO3, BSP_PMOD1_IO4},
    },
};

static const board_res_desc_t g_board_box_res = {

    .FUNC_SDMMC_EN =   (1),
    .SDMMC_BUS_WIDTH = (4),
    .GPIO_SDMMC_CLK =  (BSP_PMOD2_IO3),
    .GPIO_SDMMC_CMD =  (BSP_PMOD2_IO2),
    .GPIO_SDMMC_D0 =   (BSP_PMOD2_IO8),
    .GPIO_SDMMC_D1 =   (BSP_PMOD2_IO4),
    .GPIO_SDMMC_D2 =   (BSP_PMOD2_IO1),
    .GPIO_SDMMC_D3 =   (BSP_PMOD2_IO5),
    .GPIO_SDMMC_DET =  (GPIO_NUM_NC),

    .FUNC_SDSPI_EN =       (0),
    .SDSPI_HOST =          (SPI2_HOST),
    .GPIO_SDSPI_CS =       (GPIO_NUM_NC),
    .GPIO_SDSPI_SCLK =     (GPIO_NUM_NC),
    .GPIO_SDSPI_MISO =     (GPIO_NUM_NC),
    .GPIO_SDSPI_MOSI =     (GPIO_NUM_NC),

    .FUNC_SPI_EN =         (0),
    .GPIO_SPI_CS =         (GPIO_NUM_NC),
    .GPIO_SPI_MISO =       (GPIO_NUM_NC),
    .GPIO_SPI_MOSI =       (GPIO_NUM_NC),
    .GPIO_SPI_SCLK =       (GPIO_NUM_NC),

    .FUNC_RMT_EN =         (0),
    .GPIO_RMT_IR =         (GPIO_NUM_NC),
    .GPIO_RMT_LED =        (GPIO_NUM_39),

    .PMOD1 = &g_pmod[0],
    .PMOD2 = &g_pmod[1],
};

static esp_codec_dev_handle_t play_dev_handle;
static esp_codec_dev_handle_t record_dev_handle;

static button_handle_t *g_btn_handle = NULL;
static bsp_bottom_property_t g_bottom_handle;

static const boards_info_t g_boards_info = {
#ifdef CONFIG_BSP_BOARD_ESP32_S3_BOX_3
    .name =         "S3_BOX_3",
#elif CONFIG_BSP_BOARD_ESP32_S3_BOX
    .name =         "S3_BOX",
#else
    .name =         "S3_BOX_LITE",
#endif
    .board_desc =   &g_board_box_res
};

static const char *TAG = "bsp_board";

esp_err_t bsp_btn_init(void)
{
    ESP_ERROR_CHECK((NULL != g_btn_handle));

    int btn_num = 0;
    g_btn_handle = calloc(sizeof(button_handle_t), BSP_BUTTON_NUM);
    assert((g_btn_handle) && "memory is insufficient for button");
    return bsp_iot_button_create(g_btn_handle, &btn_num, BSP_BUTTON_NUM);
}

esp_err_t bsp_btn_register_callback(bsp_button_t btn, button_event_t event, button_cb_t callback, void *user_data)
{
    assert((g_btn_handle) && "button not initialized");
    assert((btn < BSP_BUTTON_NUM) && "button id incorrect");

    if (NULL == callback) {
        return iot_button_unregister_cb(g_btn_handle[btn], event);
    }
    return iot_button_register_cb(g_btn_handle[btn], event, callback, user_data);
}

esp_err_t bsp_btn_rm_all_callback(bsp_button_t btn)
{
    assert((g_btn_handle) && "button not initialized");
    assert((btn < BSP_BUTTON_NUM) && "button id incorrect");

    for (size_t event = 0; event < BUTTON_EVENT_MAX; event++) {
        iot_button_unregister_cb(g_btn_handle[btn], event);
    }
    return ESP_OK;
}

esp_err_t bsp_btn_rm_event_callback(bsp_button_t btn, size_t event)
{
    assert((g_btn_handle) && "button not initialized");
    assert((btn < BSP_BUTTON_NUM) && "button id incorrect");

    iot_button_unregister_cb(g_btn_handle[btn], event);
    return ESP_OK;
}

esp_err_t bsp_i2s_read(void *audio_buffer, size_t len, size_t *bytes_read, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;
    ret = esp_codec_dev_read(record_dev_handle, audio_buffer, len);
    *bytes_read = len;
    return ret;
}

esp_err_t bsp_i2s_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;
    ret = esp_codec_dev_write(play_dev_handle, audio_buffer, len);
    *bytes_written = len;
    return ret;
}

esp_err_t bsp_codec_set_fs(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    esp_err_t ret = ESP_OK;

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = rate,
        .channel = ch,
        .bits_per_sample = bits_cfg,
    };

    if (play_dev_handle) {
        ret = esp_codec_dev_close(play_dev_handle);
    }
    if (record_dev_handle) {
        ret |= esp_codec_dev_close(record_dev_handle);
        ret |= esp_codec_dev_set_in_gain(record_dev_handle, CODEC_DEFAULT_ADC_VOLUME);
    }

    if (record_dev_handle) {
        ret |= esp_codec_dev_open(play_dev_handle, &fs);
    }
    if (record_dev_handle) {
        ret |= esp_codec_dev_open(record_dev_handle, &fs);
    }
    return ret;
}

esp_err_t bsp_codec_volume_set(int volume, int *volume_set)
{
    esp_err_t ret = ESP_OK;
    float v = volume;
    ret = esp_codec_dev_set_out_vol(play_dev_handle, (int)v);
    return ret;
}

esp_err_t bsp_codec_mute_set(bool enable)
{
    esp_err_t ret = ESP_OK;
    ret = esp_codec_dev_set_out_mute(play_dev_handle, enable);
    return ret;
}

esp_err_t bsp_codec_dev_stop(void)
{
    esp_err_t ret = ESP_OK;

    if (play_dev_handle) {
        ret = esp_codec_dev_close(play_dev_handle);
    }

    if (record_dev_handle) {
        ret = esp_codec_dev_close(record_dev_handle);
    }
    return ret;
}

esp_err_t bsp_codec_dev_resume(void)
{
    return bsp_codec_set_fs(CODEC_DEFAULT_SAMPLE_RATE, CODEC_DEFAULT_BIT_WIDTH, CODEC_DEFAULT_CHANNEL);
}

static esp_err_t bsp_codec_init()
{
    play_dev_handle = bsp_audio_codec_speaker_init();
    assert((play_dev_handle) && "play_dev_handle not initialized");

    record_dev_handle = bsp_audio_codec_microphone_init();
    assert((record_dev_handle) && "record_dev_handle not initialized");

    bsp_codec_set_fs(CODEC_DEFAULT_SAMPLE_RATE, CODEC_DEFAULT_BIT_WIDTH, CODEC_DEFAULT_CHANNEL);
    return ESP_OK;
}

const boards_info_t *bsp_board_get_info(void)
{
    return &g_boards_info;
}

const board_res_desc_t *bsp_board_get_description(void)
{
    return g_boards_info.board_desc;
}

bsp_bottom_property_t *bsp_board_get_sensor_handle(void)
{
    return &g_bottom_handle;
}

__attribute__((weak)) void mute_btn_handler(void *handle, void *arg)
{
    button_event_t event = (button_event_t)arg;

    if (BUTTON_PRESS_DOWN == event) {
        esp_rom_printf(DRAM_STR("Mute On\r\n"));
    } else {
        esp_rom_printf(DRAM_STR("Mute Off\r\n"));
    }
}

esp_err_t bsp_board_init(void)
{
    esp_err_t ret = ESP_OK;

    ESP_LOGD(TAG, "Board init");

    ESP_ERROR_CHECK(bsp_btn_init());
#if !CONFIG_BSP_BOARD_ESP32_S3_BOX_Lite
    ESP_ERROR_CHECK(bsp_btn_register_callback(BSP_BUTTON_MUTE, BUTTON_PRESS_DOWN, mute_btn_handler, (void *)BUTTON_PRESS_DOWN));
    ESP_ERROR_CHECK(bsp_btn_register_callback(BSP_BUTTON_MUTE, BUTTON_PRESS_UP, mute_btn_handler, (void *)BUTTON_PRESS_UP));
#endif

    ESP_ERROR_CHECK(bsp_codec_init());
    bsp_sensor_init(&g_bottom_handle);
    return ret;
}
