/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <string.h>
#include "driver/gpio.h"
#include "ES8388_interface.h"
#include "es8311.h"
#include "es7210.h"
#include "es7243.h"
#include "esp_log.h"
#include "MediaHal.h"
#include "driver/i2s.h"
#include "driver/i2c.h"
// #include "lock.h"
// #include "InterruptionSal.h"
#if defined CONFIG_ESP32_S3_CUSTOMER_BOARD || defined CONFIG_ESP_KORVO_MIX_B_V2_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V2_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V3_0_BOARD || defined CONFIG_ESP32_S3_CUBE_V2_0_BOARD
#include "esp32s3/rom/gpio.h"
#endif
#define HAL_TAG "MEDIA_HAL"

#define MEDIA_HAL_CHECK_NULL(a, format, b, ...) \
    if ((a) == 0) { \
        ESP_LOGE(HAL_TAG, format, ##__VA_ARGS__); \
        return b;\
    }

#define I2S_OUT_VOL_DEFAULT     60
static char MUSIC_BITS = 16; //for re-bits feature, but only for 16 to 32
static int __attribute__((unused)) AMPLIFIER = 1 << 8;//amplify the volume, fixed point

typedef struct {
    i2c_port_t i2c_port_num;
    i2c_config_t i2c_cfg;
} I2CConfig;

struct media_hal {
    MediaHalState sMediaHalState;
    int amplifier_type;
    CodecMode _currentMode;
    xSemaphoreHandle _halLock;
    int (*codec_init)(void *cfg);
    void (*codec_uninit)(void);
    int (*codec_sart)(int mode);
    int (*codec_stop)(int mode);
    int (*codec_config_fmt)(int mode, int fmt);
    int (*codec_set_bit)(int mode, int bitPerSample);
    int (*codec_set_adc_input)(int input);
    int (*codec_set_vol)(int volume);
    int (*codec_get_vol)(int *volume);
    int (*codec_set_mute)(int en);
    int (*codec_get_mute)(int *mute);
#if USE_ADC
    int (*adc_init)(void);
#endif
};

struct media_hal MediaHalConfig = {
    .sMediaHalState = 0,
    .amplifier_type = 0,
#if defined CONFIG_CODEC_CHIP_IS_ES8388
    .codec_init = Es8388Init,
    .codec_uninit = Es8388Uninit,
    .codec_sart = Es8388Start,
    .codec_stop = Es8388Stop,
    .codec_config_fmt = Es8388ConfigFmt,
    .codec_set_bit = Es8388SetBitsPerSample,
    .codec_set_adc_input = Es8388ConfigAdcInput,
    .codec_set_vol = Es8388SetVoiceVolume,
    .codec_get_vol = Es8388GetVoiceVolume,
    .codec_set_mute = Es8388SetVoiceMute,
    .codec_get_mute = Es8388GetVoiceMute,
#elif defined CONFIG_CODEC_CHIP_IS_ES8311
    .codec_init = (int (*)(void *)) Es8311Init,
    .codec_uninit = Es8311Uninit,
    .codec_sart = Es8311Start,
    .codec_stop = Es8311Stop,
    .codec_config_fmt = Es8311ConfigFmt,
    .codec_set_bit = Es8311SetBitsPerSample,
    .codec_set_adc_input = Es8311ConfigAdcInput,
    .codec_set_vol = Es8311SetVoiceVolume,
    .codec_get_vol = Es8311GetVoiceVolume,
    .codec_set_mute = Es8311SetVoiceMute,
    .codec_get_mute = Es8311GetVoiceMute,
#endif
#if USE_ADC
#ifdef CONFIG_ADC_CHIP_IS_ES7243
    .adc_init = Es7243Init,
#elif defined CONFIG_ADC_CHIP_IS_ES7210
    .adc_init = Es7210Init,
#endif
#endif
};

static int I2cInit(i2c_config_t *conf, int i2cMasterPort)
{
    int res;

    res = i2c_param_config(i2cMasterPort, conf);
    res |= i2c_driver_install(i2cMasterPort, conf->mode, 0, 0, 0);
    if (res < 0) {
        ESP_LOGE(HAL_TAG, "I2cInit error");
    }
    return res;
}
#if defined CONFIG_ESP32_S3_CUSTOMER_BOARD || defined CONFIG_ESP32_S3_KORVO_V1_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V2_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V3_0_BOARD || defined CONFIG_ESP32_S3_CUBE_V2_0_BOARD
void i2s_mclK_matrix_out(int i2s_num, int io_num)
{
#include "driver/gpio.h"
#include "esp_rom_gpio.h"
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[io_num], PIN_FUNC_GPIO);
    gpio_set_direction(io_num, GPIO_MODE_OUTPUT);
    esp_rom_gpio_connect_out_signal(io_num, i2s_num == 0 ? 23 : 21, 0, 0);
}
#endif
static void __attribute__((unused)) i2s_mclk_matrix_out(int i2s_num, int gpio)
{
    int signal_idx = (i2s_num == 1) ? 21 : 23;
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[gpio], PIN_FUNC_GPIO);//
    gpio_set_direction(gpio, GPIO_MODE_DEF_OUTPUT);//
    gpio_matrix_out(gpio, signal_idx, 0, 0);
}

int MediaHalInit(void *config)
{
    int ret  = 0;

    // Codec
#if USE_I2S_0
    i2s_config_t i2s0_config = I2S0_CONFIG();

    i2s_pin_config_t i2s0_pin = I2S0_PIN();
    ret = i2s_driver_install(I2S_NUM_0, &i2s0_config, 0, NULL);
    ret |= i2s_set_pin(I2S_NUM_0, &i2s0_pin);
    if (ret < 0) {
        ESP_LOGE(HAL_TAG, "I2S_NUM_0 install failed");
        return -1;
    }
#endif

#if USE_I2S_1
    i2s_config_t i2s1_config = I2S1_CONFIG();

    i2s_pin_config_t i2s1_pin = I2S1_PIN();
    ret = i2s_driver_install(I2S_NUM_1, &i2s1_config, 0, NULL);
    ret |= i2s_set_pin(I2S_NUM_1, &i2s1_pin);
    if (ret < 0) {
        ESP_LOGE(HAL_TAG, "I2S_NUM_1 install failed");
        return -1;
    }
#endif
// MCLK

#ifdef CONFIG_IDF_TARGET_ESP32S2
    gpio_matrix_out(35, CLK_I2S_MUX_IDX, 0, 0);
#endif

#ifdef ENABLE_MCLK_GPIO0
    SET_PERI_REG_BITS(PIN_CTRL, CLK_OUT1, 0, CLK_OUT1_S);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);
#endif
#if defined CONFIG_ESP32_S3_KORVO_V1_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V2_0_BOARD
    i2s_mclk_matrix_out(0, 42);
    i2s_mclK_matrix_out(1, 20);
#endif

#if defined CONFIG_ESP32_S3_KORVO_V3_0_BOARD
    #include "soc/usb_serial_jtag_reg.h"
    CLEAR_PERI_REG_MASK(USB_SERIAL_JTAG_CONF0_REG, USB_SERIAL_JTAG_USB_PAD_ENABLE);
    i2s_mclK_matrix_out(0, 42);
    i2s_mclK_matrix_out(1, 20);
#endif

#if defined CONFIG_ESP32_S3_CUBE_V2_0_BOARD
    #include "soc/usb_serial_jtag_reg.h"
    i2s_mclK_matrix_out(1, 14);
#endif

#if CONFIG_ESP32_S3_CUSTOMER_BOARD
    i2s_mclK_matrix_out(0, 48);
    i2s_mclK_matrix_out(1, 20);
#endif


#if defined CONFIG_ESP_KORVO_MIX_B_V1_0_BOARD || defined CONFIG_ESP_KORVO_MIX_B_V2_0_BOARD
    i2s_mclk_matrix_out(1, 0);
#endif

    // I2C INIT
    I2CConfig  I2CCfg = I2C_CONFIG() ;
    I2cInit(&(I2CCfg.i2c_cfg), I2CCfg.i2c_port_num);
#if USE_DUAL_I2C
    I2CConfig  I2CCfg1 = I2C1_CONFIG() ;
    I2cInit(&(I2CCfg1.i2c_cfg), I2CCfg1.i2c_port_num);
#endif

#if USE_CODEC
    ret |= MediaHalConfig.codec_init(config);
    ret |= MediaHalConfig.codec_config_fmt(ES_MODULE_ADC_DAC, ES_I2S_NORMAL);
    ret |= MediaHalConfig.codec_set_bit(ES_MODULE_ADC_DAC, BIT_LENGTH_16BITS);
    ret |= MediaHalConfig.codec_set_adc_input(ADC_INPUT_LINPUT2_RINPUT2);
    ret |= MediaHalConfig.codec_sart(ES_MODULE_ADC_DAC);
    ret |= MediaHalConfig.codec_set_vol(I2S_OUT_VOL_DEFAULT);
    ESP_LOGI(HAL_TAG, "I2S_OUT_VOL_DEFAULT[%d]", I2S_OUT_VOL_DEFAULT);
#endif

#if USE_ADC
    ret |= MediaHalConfig.adc_init();
#endif

    MediaHalConfig._currentMode = CODEC_MODE_UNKNOWN;
    if (MediaHalConfig._halLock) {
        // mutex_destroy(MediaHalConfig._halLock);
    }
    // MediaHalConfig._halLock = mutex_init();
    MediaHalConfig.sMediaHalState = MEDIA_HAL_STATE_INIT;

    // Init PA
#ifdef GPIO_PA_EN
    gpio_config_t  io_conf;
    memset(&io_conf, 0, sizeof(io_conf));
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = ((1ULL << GPIO_PA_EN));
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(GPIO_PA_EN, 1);
#endif
    // printf("i2s init done!!!!\n");

    return ret;


}

int MediaHalUninit(void)
{
    // mutex_destroy(MediaHalConfig._halLock);
    MediaHalConfig.codec_uninit();
    MediaHalConfig._halLock = NULL;
    MUSIC_BITS = 0;
    MediaHalConfig._currentMode = CODEC_MODE_UNKNOWN;
    MediaHalConfig.sMediaHalState = MEDIA_HAL_STATE_UNKNOWN;
    return 0;
}

int MediaHalSetVolume(int volume)
{
    int ret = 0;
    // mutex_lock(MediaHalConfig._halLock);
    int mute;
    MediaHalConfig.codec_get_mute(&mute);
    if (volume < 3 ) {
        if (0 == mute) {
            MediaHalConfig.codec_set_mute(CODEC_MUTE_ENABLE);
        }
    } else {
        if ((1 == mute)) {
            MediaHalConfig.codec_set_mute(CODEC_MUTE_DISABLE);
        }
    }
    ret = MediaHalConfig.codec_set_vol(volume);

    // mutex_unlock(MediaHalConfig._halLock);

    return ret;
}

int MediaHalGetVolume(int *volume)
{
    int ret = 0;
    MEDIA_HAL_CHECK_NULL(volume, "Get volume para is null", -1);
    // mutex_lock(MediaHalConfig._halLock);
    ret = MediaHalConfig.codec_get_vol(volume);
    // mutex_unlock(MediaHalConfig._halLock);
    return ret;
}

int MediaHalSetMute(CodecMute mute)
{
    int ret = 0;
    // mutex_lock(MediaHalConfig._halLock);
    ret = MediaHalConfig.codec_set_mute(mute);
    // mutex_unlock(MediaHalConfig._halLock);

    return ret;
}

int MediaHalGetMute(void)
{
    int mute = 0;
    // mutex_lock(MediaHalConfig._halLock);
    MediaHalConfig.codec_get_mute(&mute);
    // mutex_unlock(MediaHalConfig._halLock);

    return mute;
}

void codec_init(void)
{
    // Init Codec or DAC
    int ret = 0;
#if USE_CODEC
#if (defined CONFIG_CODEC_CHIP_IS_ES8388)
    CodecConfig  CodeCfg =  AUDIO_CODEC_ES8388_DEFAULT();
    ESP_LOGI(HAL_TAG, "CONFIG_CODEC_CHIP_IS_ES8388");
#elif (defined CONFIG_CODEC_CHIP_IS_ES8311)
    CodecConfig  CodeCfg =  AUDIO_CODEC_ES8311_DEFAULT();
    ESP_LOGI(HAL_TAG, "CONFIG_CODEC_CHIP_IS_ES8311");
#endif

    ret = MediaHalInit(&CodeCfg);
    if (ret) {
        ESP_LOGE(HAL_TAG, "MediaHal init failed, line:%d", __LINE__);
    }
    MediaHalSetVolume(65);
#else
    ret = MediaHalInit(NULL);
    if (ret) {
        ESP_LOGE(HAL_TAG, "MediaHal init failed, line:%d", __LINE__);
    }

#endif
}