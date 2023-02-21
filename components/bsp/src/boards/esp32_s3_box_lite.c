/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include "esp_log.h"
#include "bsp_board.h"

#include "esp_log.h"
#include "esp_check.h"
#include "hal/i2s_hal.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "soc/soc_caps.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#else
#include "driver/adc.h"
#include "esp_adc_cal.h"
#endif

#include "bsp_board.h"
#include "iot_button.h"


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

static const board_res_desc_t g_board_s3_box_lite_res = {

    .FUNC_SDMMC_EN =   (1),
    .SDMMC_BUS_WIDTH = (4),
    .GPIO_SDMMC_CLK =  (GPIO_NUM_13),
    .GPIO_SDMMC_CMD =  (GPIO_NUM_11),
    .GPIO_SDMMC_D0 =   (GPIO_NUM_14),
    .GPIO_SDMMC_D1 =   (GPIO_NUM_12),
    .GPIO_SDMMC_D2 =   (GPIO_NUM_10),
    .GPIO_SDMMC_D3 =   (GPIO_NUM_9),
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


const button_config_t BOARD_BTN_ID_config[BOARD_BTN_ID_NUM] = {
    {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config.active_level = false,
        .gpio_button_config.gpio_num = BOARD_BTN_ID_BOOT,
    },
    {
        .type = BUTTON_TYPE_ADC,
        .adc_button_config.adc_channel = ADC_CHANNEL_0, // ADC1 channel 0 is GPIO1
        .adc_button_config.button_index = BOARD_BTN_ID_PREV,
        .adc_button_config.min = 2286, // middle is 2386mV
        .adc_button_config.max = 2486
    },
    {
        .type = BUTTON_TYPE_ADC,
        .adc_button_config.adc_channel = ADC_CHANNEL_0, // ADC1 channel 0 is GPIO1
        .adc_button_config.button_index = BOARD_BTN_ID_ENTER,
        .adc_button_config.min = 1863, // middle is 1963mV
        .adc_button_config.max = 2063
    },
    {
        .type = BUTTON_TYPE_ADC,
        .adc_button_config.adc_channel = ADC_CHANNEL_0, // ADC1 channel 0 is GPIO1
        .adc_button_config.button_index = BOARD_BTN_ID_NEXT,
        .adc_button_config.min = 694, // middle is 794mV
        .adc_button_config.max = 894
    }
};

static button_handle_t *g_btn_handle = NULL;
static bsp_codec_config_t g_codec_handle;

static const boards_info_t g_boards_info = {
    .id =           BOARD_S3_BOX_LITE,
    .name =         "S3_BOX_LITE",
    .board_desc =   &g_board_s3_box_lite_res
};

static i2s_chan_handle_t i2s_tx_chan;
static i2s_chan_handle_t i2s_rx_chan;

static const char *TAG = "board";


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
    ret = i2s_channel_read(i2s_rx_chan, (char *)audio_buffer, len, bytes_read, timeout_ms);
    return ret;
}

static esp_err_t bsp_i2s_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms)
{
    esp_err_t ret = ESP_OK;
    ret = i2s_channel_write(i2s_tx_chan, (char *)audio_buffer, len, bytes_written, timeout_ms);
    return ret;
}

static esp_err_t bsp_i2s_reconfig_clk(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch)
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

static esp_err_t bsp_codec_volume_set(int volume, int *volume_set)
{
    esp_err_t ret = ESP_OK;
    return ret;
}

static esp_err_t bsp_codec_mute_set(bool enable)
{
    esp_err_t ret = ESP_OK;
    return ret;
}

static esp_err_t bsp_codec_es7243_set()
{
    esp_err_t ret = ESP_OK;
    return ret;
}

static void bsp_codec_init()
{
    /* Configure I2S peripheral and Power Amplifier */
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = BSP_I2S_GPIO_CFG,
    };
    bsp_audio_init(&std_cfg, &i2s_tx_chan, &i2s_rx_chan);
    bsp_audio_poweramp_enable(true);

    bsp_codec_config_t *codec_config = bsp_board_get_codec_handle();
    codec_config->volume_set_fn = bsp_codec_volume_set;
    codec_config->mute_set_fn = bsp_codec_mute_set;
    codec_config->codec_reconfig_fn = bsp_codec_es7243_set;
    codec_config->i2s_read_fn = bsp_i2s_read;
    codec_config->i2s_write_fn = bsp_i2s_write;
    codec_config->i2s_reconfig_clk_fn = bsp_i2s_reconfig_clk;
}

esp_err_t bsp_board_s3_box_lite_init(void)
{
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

    ret |= bsp_board_s3_box_lite_init();

    return ret;
}
