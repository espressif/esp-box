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

#ifndef _AUDIO_KORVO_MIX_A_V1_0_BOARD_H_
#define _AUDIO_KORVO_MIX_A_V1_0_BOARD_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Board functions related */
#define BOARD_INFO                  "ESP_KORVO_MIX_A_V1_0_BOARD"
#define USE_CODEC                   1
#define USE_ADC                     0
#define CONFIG_CODEC_CHIP_IS_ES8311
#define USE_I2S_0                   1
#define USE_I2S_1                   0

/* I2C gpios */
#define IIC_CLK                     32
#define IIC_DATA                    19

/* PA */
#define GPIO_PA_EN                  12

/* I2S gpios */
#define IIS_MCLK                    0
#define IIS_SCLK                    25
#define IIS_LCLK                    22
#define IIS_DSIN                    13
#define IIS_DOUT                    -1

#define I2S0_CONFIG(){ \
    .mode = I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX, \
    .sample_rate = 16000, \
    .bits_per_sample = 16, \
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, \
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
