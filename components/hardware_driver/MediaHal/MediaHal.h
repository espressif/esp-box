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

#ifndef _MEDIA_HAL_H_
#define _MEDIA_HAL_H_

typedef enum CodecMode {
    CODEC_MODE_UNKNOWN,
    CODEC_MODE_ENCODE ,
    CODEC_MODE_DECODE ,
    CODEC_MODE_LINE_IN,
    CODEC_MODE_DECODE_ENCODE,
    CODEC_MODE_MAX
} CodecMode;

typedef enum CodecMute {
    CODEC_MUTE_DISABLE,
    CODEC_MUTE_ENABLE,
} CodecMute;


typedef enum MediaHalState {
    MEDIA_HAL_STATE_UNKNOWN,
    MEDIA_HAL_STATE_INIT,
    MEDIA_HAL_STATE_MAX,
} MediaHalState;

/**
 * @brief Initialize media codec driver.
 *
 * @praram config : setting parameters
 *
 * @return  int, 0--success, others--fail
 */
int MediaHalInit(void* config);

/**
 * @brief Uninitialize media codec driver.
 *
 * @praram void
 *
 * @return  int, 0--success, others--fail
 */
int MediaHalUninit(void);

/**
 * @brief Start codec driver.
 *
 * @param  mode : Refer to enum CodecMode.
 *
 * @return     int, 0--success, others--fail
 */
int MediaHalStart(CodecMode mode);

/**
 * @brief Stop codec driver.
 *
 * @param  mode : Refer to enum CodecMode.
 *
 * @return     int, 0--success, others--fail
 */
int MediaHalStop(CodecMode mode);

/**
 * @brief Get the codec working mode.
 *
 * @param  mode: Current working mode will be return.
 *
 * @return     int, 0--success, others--fail
 */
int MediaHalGetCurrentMode(CodecMode *mode);

/**
 * @brief Set voice volume.
 *
 * @param  volume: Value will be setup.
 *
 * @return     int, 0--success, others--fail
 */
int MediaHalSetVolume(int volume);

/**
 * @brief Get voice volume.
 *
 * @param  volume:Current volume will be return.
 *
 * @return     int, 0--success, others--fail
 */
int MediaHalGetVolume(int *volume);

/**
 * @brief Set the volume amplifier. This function is called when external ADC can not change volume or
 *          when users want to amplify the max volume.
 *
 * @param  scale: scale the voice volume, such as 0.5, or 1.3
 */
void MediaHalSetVolumeAmplify(float scale);

/**
 * @brief Get the volume amplifier
 *
 * @return     int, a fixed point (1<<8) value
 */
int MediaHalGetVolumeAmplify();

/**
 * @brief Get the amplifier type
 *
 * @return     int, 0: hardware codec, 1:software
 */
int MediaHalGetAmplifyType();

/**
 * @brief Set codec driver mute status.
 *
 * @param  mute: 1--Enable mute; 0-- Disable mute.
 *
 * @return     int, 0--success, others--fail
 */
int MediaHalSetMute(CodecMute mute);

/**
 * @brief Get codec driver mute status.
 *
 * @param  void
 *
 * @return     int, 0-- Unmute, 1-- Mute, <0 --fail
 */
int MediaHalGetMute(void);

/**
 * @brief Set codec Bits.
 *
 * @param  bitPerSample: see structure BitsLength
 *
 * @return     int, 0-- success, -1 --fail
 */
int MediaHalSetBits(int bitPerSample);

/**
 * @brief Set clock & bit width used for I2S RX and TX.
 *
 * @param i2s_num  I2S_NUM_0, I2S_NUM_1
 *
 * @param rate I2S sample rate (ex: 8000, 44100...)
 *
 * @param bits I2S bit width (16, 24, 32)
 *
 * @return
 *     - 0   Success
 *     - -1  Error
 */
int MediaHalSetClk(int i2s_num, uint32_t rate, uint8_t bits, uint32_t ch);

/**
 * @brief Get i2s configuration.
 *
 * @param i2s_num  I2S_NUM_0, I2S_NUM_1
 * @param info: i2s_config_t information.
 *
 * @return     int, 0-- success, -1 --fail
 */
int MediaHalGetI2sConfig(int i2sNum, void *info);

/**
 * @brief Get i2s number
 *
 * @param  None
 *
 * @return int, i2s number
 */
int MediaHalGetI2sNum(void);

/**
 * @brief Get i2s bits
 *
 * @param  None
 *
 * @return int, i2s bits
 */
int MediaHalGetI2sBits(void);

/**
 * @brief get source-music bits for re-bits feature, but only for 16bits to 32bits
 *
 * @param  None
 *
 * @return
 *     - 0   disabled
 *     - 16  enabled
 */
int MediaHalGetSrcBits(void);

/**
 * @brief get whether i2s is in DAC mode
 *
 * @param  None
 *
 * @return
 *     - 0   not in DAC mode
 *     - 1   is DAC mode
 */
int MediaHalGetI2sDacMode(void);

/**
 * @brief get whether i2s is in ADC mode
 *
 * @param  None
 *
 * @return
 *     - 0   not in ADC mode
 *     - 1   is ADC mode
 */
int MediaHalGetI2sAdcMode(void);

/**
 * @brief Power on or not for PA.
 *
 * @param  1: enable; 0: disable
 *
 * @return     int, 0-- success, -1 --fail
 */
int MediaHalPaPwr(int en);

/**
 * @brief Get MediaHal state.
 *
 * @param  state: MediaHalState enum
 *
 * @return     int, 0-- success, -1 --fail
 */
int MediaHalGetState(MediaHalState *state);

void codec_init(void);

#endif  //__MEDIA_HAL_H__
