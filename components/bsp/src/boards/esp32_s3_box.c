/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_log.h"
#include "esp_check.h"

#include "bsp/esp-bsp.h"
#include "bsp_board.h"
#include "iot_button.h"

#define ES8311_SAMPLE_RATE          (16000)
#define ES8311_DEFAULT_VOLUME       (60)

#define ES7210_SAMPLE_RATE          (16000)
#define ES7210_BIT_WIDTH            (16)
#define ES7210_ADC_VOLUME           (24.0)
#define ES7210_CHANNEL              (2)

static uint8_t bsp_home_button_get(void *param);

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

static const board_res_desc_t g_board_s3_box_res = {

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

static const button_config_t BOARD_BTN_ID_config[BOARD_BTN_ID_NUM] = {
    {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config.active_level = false,
        .gpio_button_config.gpio_num = BOARD_BTN_ID_BOOT,
    },
    {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config.active_level = false,
        .gpio_button_config.gpio_num = BOARD_BTN_ID_MUTE,
    },
    {
        .type = BUTTON_TYPE_CUSTOM,
        .custom_button_config.priv = NULL,
        .custom_button_config.button_custom_get_key_value = bsp_home_button_get,
        .custom_button_config.active_level = true,
    },
};


static esp_codec_dev_handle_t play_dev_handle;
static esp_codec_dev_handle_t record_dev_handle;

static button_handle_t *g_btn_handle = NULL;
static bsp_codec_config_t g_codec_handle;

static const boards_info_t g_boards_info = {
    .id =           BOARD_S3_BOX,
    .name =         "S3_BOX",
    .board_desc =   &g_board_s3_box_res
};

static const char *TAG = "board";

static uint8_t bsp_home_button_get(void *param)
{
#if CONFIG_BSP_TOUCH_BUTTON
    return bsp_button_get(BSP_BUTTON_MAIN);
#else
    return false;
#endif
}

esp_err_t bsp_btn_init(void)
{
    ESP_ERROR_CHECK((NULL != g_btn_handle));

    g_btn_handle = calloc(sizeof(button_handle_t), BOARD_BTN_ID_NUM);
    assert((g_btn_handle) && "memory is insufficient for button");

    ESP_LOGI(TAG, "[+ Btn Init] ID");
    /* Init buttons */
    for (int i = 0; i < BOARD_BTN_ID_NUM; i++) {
        g_btn_handle[i] = iot_button_create(&BOARD_BTN_ID_config[i]);
        assert(g_btn_handle[i]);
    }
    return ESP_OK;
}

esp_err_t bsp_btn_register_callback(bsp_button_id_t btn, button_event_t event, button_cb_t callback, void *user_data)
{
    assert((g_btn_handle) && "button not initialized");
    assert((btn < BOARD_BTN_ID_NUM) && "button id incorrect");

    ESP_LOGI(TAG, "[+ register] ID:%d, event:%d", btn, event);

    if (NULL == callback) {
        return iot_button_unregister_cb(g_btn_handle[btn], event);
    }

    return iot_button_register_cb(g_btn_handle[btn], event, callback, user_data);
}

esp_err_t bsp_btn_rm_all_callback(bsp_button_id_t btn)
{
    assert((g_btn_handle) && "button not initialized");
    assert((btn < BOARD_BTN_ID_NUM) && "button id incorrect");

    ESP_LOGI(TAG, "[- register] ID:%d", btn);

    for (size_t event = 0; event < BUTTON_EVENT_MAX; event++) {
        iot_button_unregister_cb(g_btn_handle[btn], event);
    }
    return ESP_OK;
}

static esp_err_t bsp_i2s_read(void *audio_buffer, size_t len, size_t *bytes_read, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;
    ret = esp_codec_dev_read(record_dev_handle, audio_buffer, len);
    *bytes_read = len;
    return ret;
}

static esp_err_t bsp_i2s_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;
    ret = esp_codec_dev_write(play_dev_handle, audio_buffer, len);
    *bytes_written = len;
    return ret;
}

static esp_err_t bsp_codec_es8311_set(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
{
    esp_err_t ret = ESP_OK;

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = rate,
        .channel = ch,
        .bits_per_sample = bits_cfg,
    };

    ret = esp_codec_dev_close(play_dev_handle);
    ret = esp_codec_dev_close(record_dev_handle);

    ret = esp_codec_dev_open(play_dev_handle, &fs);
    ret = esp_codec_dev_open(record_dev_handle, &fs);
    return ret;
}

static esp_err_t bsp_codec_volume_set(int volume, int *volume_set)
{
    esp_err_t ret = ESP_OK;
    float v = volume;
    v *= 0.6f;
    v = -0.01f * (v * v) + 2.0f * v;
    ret = esp_codec_dev_set_out_vol(play_dev_handle, (int)v);
    return ret;
}

static esp_err_t bsp_codec_mute_set(bool enable)
{
    esp_err_t ret = ESP_OK;
    ret = esp_codec_dev_set_out_mute(play_dev_handle, enable);
    return ret;
}

static esp_err_t bsp_codec_es7210_set()
{
    esp_err_t ret = ESP_OK;

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = ES7210_SAMPLE_RATE,
        .channel = ES7210_CHANNEL,
        .bits_per_sample = ES7210_BIT_WIDTH,
    };

    assert(record_dev_handle);
    ret = esp_codec_dev_open(record_dev_handle, &fs);
    esp_codec_dev_set_in_gain(record_dev_handle, ES7210_ADC_VOLUME);
    return ret;
}

static void bsp_codec_init()
{
    play_dev_handle = bsp_audio_codec_speaker_init();
    assert((play_dev_handle) && "play_dev_handle not initialized");

    record_dev_handle = bsp_audio_codec_microphone_init();
    assert((record_dev_handle) && "record_dev_handle not initialized");

    bsp_audio_poweramp_enable(false);

    bsp_codec_es7210_set();

    bsp_codec_config_t *codec_config = bsp_board_get_codec_handle();
    codec_config->volume_set_fn = bsp_codec_volume_set;
    codec_config->mute_set_fn = bsp_codec_mute_set;
    codec_config->codec_reconfig_fn = bsp_codec_es7210_set;
    codec_config->i2s_read_fn = bsp_i2s_read;
    codec_config->i2s_write_fn = bsp_i2s_write;
    codec_config->i2s_reconfig_clk_fn = bsp_codec_es8311_set;
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

esp_err_t bsp_board_s3_box_init(void)
{
    bsp_btn_init();
    bsp_btn_register_callback(BOARD_BTN_ID_MUTE, BUTTON_PRESS_DOWN, mute_btn_handler, (void *)BUTTON_PRESS_DOWN);
    bsp_btn_register_callback(BOARD_BTN_ID_MUTE, BUTTON_PRESS_UP, mute_btn_handler, (void *)BUTTON_PRESS_UP);

    /**
     * @brief Initialize I2S and audio codec
     *
     * @note Actually the sampling rate can be reconfigured.
     *       `MP3GetLastFrameInfo` can fill the `MP3FrameInfo`, which includes `samprate`.
     *       So theoretically, the sampling rate can be dynamically changed according to the MP3 frame information.
     */
    bsp_codec_init();

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

bsp_codec_config_t *bsp_board_get_codec_handle(void)
{
    return &g_codec_handle;
}

esp_err_t bsp_board_init(void)
{
    esp_err_t ret = ESP_OK;

    ret |= bsp_board_s3_box_init();

    return ret;
}
