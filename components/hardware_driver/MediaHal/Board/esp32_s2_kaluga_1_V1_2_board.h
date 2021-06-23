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

#ifndef _AUDIO_S2_KALUGA_BOARD_H_
#define _AUDIO_S2_KALUGA_BOARD_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Board functions related */
#define BOARD_INFO                  "ESP_KORVO_V1_1_BOARD"
#define ENABLE_MCLK_GPIO0
#define USE_CODEC                   1
#define USE_ADC                     0
#define CONFIG_CODEC_CHIP_IS_ES8311
#define USE_I2S_0                   1
#define USE_I2S_1                   0

/* SD card related */
#define SD_CARD_INTR_GPIO           34
#define SD_CARD_INTR_SEL            GPIO_SEL_34

#define SD_CARD_DATA0               2
#define SD_CARD_DATA1               4
#define SD_CARD_DATA2               12
#define SD_CARD_DATA3               13
#define SD_CARD_CMD                 15
#define SD_CARD_CLK                 14

/* AUX-IN related */
#ifdef ENABLE_AUXIN_DETECT
#define AUXIN_DETECT_PIN           12
#endif

/* LED indicators */
#define GPIO_LED_GREEN              22

/* Headphone detect */
#define GPIO_HEADPHONE_DETECT       19

/* I2C gpios */
#define IIC_CLK                     7
#define IIC_DATA                    8

/* PA */
#define GPIO_PA_EN                  GPIO_NUM_10

/* Press button related */
#define GPIO_REC                    GPIO_NUM_36
#define GPIO_MODE                   GPIO_NUM_39

/* Touch pad related */
#define TOUCH_SET                   TOUCH_PAD_NUM9  // 32
#define TOUCH_PLAY                  TOUCH_PAD_NUM8  // 33
#define TOUCH_VOLUP                 TOUCH_PAD_NUM7  // 27
#define TOUCH_VOLDWN                TOUCH_PAD_NUM4  // 13

/* I2S gpios */
#define IIS_MCLK                    35

// #define IIS_SCLK                    17
// #define IIS_LCLK                    18
// #define IIS_DSIN                    14
// #define IIS_DOUT                    12
#define IIS_SCLK                    18
#define IIS_LCLK                    17
#define IIS_DSIN        12
#define IIS_DOUT        46

#define I2S0_CONFIG(){ \
    .mode = I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX, \
    .sample_rate = 16000, \
    .bits_per_sample = 16, \
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, \
    .communication_format = I2S_COMM_FORMAT_I2S, \
    .dma_buf_count = 3, \
    .dma_buf_len = 300, \
    .intr_alloc_flags = I2S_INTER_FLAG, \
    .use_apll = 1, \
};

#define I2S0_PIN(){ \
    .bck_io_num = IIS_SCLK, \
    .ws_io_num = IIS_LCLK, \
    .data_out_num = IIS_DSIN, \
    .data_in_num = IIS_DOUT, \
};

#define I2C_CONFIG(){ \
    .i2c_port_num = I2C_NUM_0, \
    .i2c_cfg = { \
        .mode = I2C_MODE_MASTER, \
        .sda_io_num = IIC_DATA, \
        .scl_io_num = IIC_CLK, \
        .sda_pullup_en = GPIO_PULLUP_ENABLE,\
        .scl_pullup_en = GPIO_PULLUP_ENABLE,\
        .master.clk_speed = 100000 \
    }, \
};

#ifdef __cplusplus
}
#endif

#endif
