/**
 * @file bsp_codec.c
 * @brief 
 * @version 0.1
 * @date 2021-07-20
 * 
 * @copyright Copyright 2021 Espressif Systems (Shanghai) Co. Ltd.
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *               http://www.apache.org/licenses/LICENSE-2.0

 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "bsp_codec.h"
#include "bsp_i2c.h"
#include "es7210.h"
#include "es8311.h"
#include "esp_log.h"

static const char *TAG = "codec";

typedef enum {
    CODEC_TYPE_ES7210 = 0,
    CODEC_TYPE_ES8311,
    CODEC_TYPE_ES8388,
    CODEC_TYPE_MAX,
    CODEC_TYPE_NONE = -1,
} codec_type_t;

typedef struct {
    uint8_t dev_addr;
    char *dev_name;
    codec_type_t dev_type;
} codec_dev_t;

static codec_dev_t codec_dev_list[] = {
    { 0x40, "ES7210", CODEC_TYPE_ES7210 },
    { 0x18, "ES8311", CODEC_TYPE_ES8311 },
    { 0x20, "ES8388", CODEC_TYPE_ES8388 },
};

static esp_err_t bsp_codec_prob(int *codec_type)
{
    for (size_t i = 0; i < sizeof(codec_dev_list) / sizeof(codec_dev_list[0]); i++) {
        if (ESP_OK == bsp_i2c_probe_addr(codec_dev_list[i].dev_addr)) {
            *codec_type |= 1 << i;
            ESP_LOGI(TAG, "Detected codec at 0x%02X. Name : %s",
                codec_dev_list[i].dev_addr, codec_dev_list[i].dev_name);
        }
    }

    if (0 == *codec_type) {
        *codec_type = CODEC_TYPE_NONE;
        ESP_LOGW(TAG, "Codec not detected");
        return ESP_ERR_NOT_FOUND;
    }

    return ESP_OK;
}

esp_err_t bsp_codec_adc_init(audio_hal_iface_samples_t sample_rate)
{
    esp_err_t ret_val = ESP_OK;
    audio_hal_codec_config_t cfg = {
        .codec_mode = AUDIO_HAL_CODEC_MODE_ENCODE,
        .adc_input = AUDIO_HAL_ADC_INPUT_ALL,
        .i2s_iface = {
            .bits = AUDIO_HAL_BIT_LENGTH_16BITS,
            .fmt = AUDIO_HAL_I2S_NORMAL,
            .mode = AUDIO_HAL_MODE_SLAVE,
            .samples = sample_rate,
        },
    };

    ret_val |= es7210_adc_init(&cfg);
    ret_val |= es7210_adc_config_i2s(cfg.codec_mode, &cfg.i2s_iface);
    ret_val |= es7210_adc_set_gain(ES7210_INPUT_MIC1 | ES7210_INPUT_MIC2, GAIN_37_5DB);
    ret_val |= es7210_adc_set_gain(ES7210_INPUT_MIC3 | ES7210_INPUT_MIC4, GAIN_0DB);
    ret_val |= es7210_adc_ctrl_state(cfg.codec_mode, AUDIO_HAL_CTRL_START);

    if (ESP_OK != ret_val) {
        ESP_LOGE(TAG, "Failed initialize codec");
    }

    return ret_val;
}

esp_err_t bsp_codec_dac_init(audio_hal_iface_samples_t sample_rate)
{
    esp_err_t ret_val = ESP_OK;
    audio_hal_codec_config_t cfg = {
        .codec_mode = AUDIO_HAL_CODEC_MODE_DECODE,
        .dac_output = AUDIO_HAL_DAC_OUTPUT_LINE1,
        .i2s_iface = {
            .bits = AUDIO_HAL_BIT_LENGTH_16BITS,
            .fmt = AUDIO_HAL_I2S_NORMAL,
            .mode = AUDIO_HAL_MODE_SLAVE,
            .samples = sample_rate,
        },
    };

    ret_val |= es8311_codec_init(&cfg);
    ret_val |= es8311_set_bits_per_sample(cfg.i2s_iface.bits);
    ret_val |= es8311_config_fmt(cfg.i2s_iface.fmt);
    ret_val |= es8311_codec_set_voice_volume(60);
    ret_val |= es8311_codec_ctrl_state(cfg.codec_mode, AUDIO_HAL_CTRL_START);

    if (ESP_OK != ret_val) {
        ESP_LOGE(TAG, "Failed initialize codec");
    }

    return ret_val;
}

esp_err_t bsp_codec_init(audio_hal_iface_samples_t sample_rate)
{
    esp_err_t ret_val = ESP_OK;

    static int codec_type = 0;

    ret_val |= bsp_codec_prob(&codec_type);

    if(CODEC_TYPE_NONE == codec_type) {
        return ESP_ERR_NOT_FOUND;
    }

    if (((1 << CODEC_TYPE_ES8311) + (1 << CODEC_TYPE_ES7210)) == codec_type) {
        ret_val |= bsp_codec_adc_init(sample_rate);
        ret_val |= bsp_codec_dac_init(sample_rate);
        return ret_val;
    }

    ESP_LOGW(TAG, "Currently not support");
    return ESP_ERR_NOT_SUPPORTED;
}
