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
#include "esp_log.h"
#include "driver/i2c.h"
#include "ES8388_interface.h"

#define ES8388_TAG "8388"

#define ES_ASSERT(a, format, b, ...) \
    if ((a) != 0) { \
        ESP_LOGE(ES8388_TAG, format, ##__VA_ARGS__); \
        return b;\
    }

#define LOG_8388(fmt, ...)   ESP_LOGW(ES8388_TAG, fmt, ##__VA_ARGS__)

/**
 * @brief Write ES8388 register
 *
 * @param slaveAdd : slave address
 * @param regAdd    : register address
 * @param data      : data to write
 *
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
static int Es8388WriteReg(uint8_t slaveAdd, uint8_t regAdd, uint8_t data)
{
    int res = 0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    res |= i2c_master_start(cmd);
    res |= i2c_master_write_byte(cmd, slaveAdd, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_write_byte(cmd, regAdd, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_write_byte(cmd, data, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_stop(cmd);
    res |= i2c_master_cmd_begin(0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    ES_ASSERT(res, "Es8388WriteReg error", -1);
    return res;
}

/**
 * @brief Read ES8388 register
 *
 * @param regAdd    : register address
 *
 * @return
 *     - (-1)     Error
 *     - (0)      Success
 */
static int Es8388ReadReg(uint8_t regAdd, uint8_t *pData)
{
    uint8_t data;
    int res = 0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    res |= i2c_master_start(cmd);
    res |= i2c_master_write_byte(cmd, ES8388_ADDR, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_write_byte(cmd, regAdd, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_stop(cmd);
    res |= i2c_master_cmd_begin(0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    cmd = i2c_cmd_link_create();
    res |= i2c_master_start(cmd);
    res |= i2c_master_write_byte(cmd, ES8388_ADDR | 0x01, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_read_byte(cmd, &data, 0x01/*NACK_VAL*/);
    res |= i2c_master_stop(cmd);
    res |= i2c_master_cmd_begin(0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    ES_ASSERT(res, "Es8388ReadReg error", -1);
    *pData = data;
    return res;
}


/**
 * @brief Inite i2s master mode
 *
 * @return
 *     - (-1)  Error
 *     - (0)  Success
 */
static int I2cInit(i2c_config_t *conf, int i2cMasterPort)
{
    int res;

    res = i2c_param_config(i2cMasterPort, conf);
    res |= i2c_driver_install(i2cMasterPort, conf->mode, 0, 0, 0);
    ES_ASSERT(res, "I2cInit error", -1);
    return res;
}

/**
 * @brief Configure ES8388 ADC and DAC volume. Basicly you can consider this as ADC and DAC gain
 *
 * @param mode:             set ADC or DAC or all
 * @param volume:           -96 ~ 0              for example Es8388SetAdcDacVolume(ES_MODULE_ADC, 30, 6); means set ADC volume -30.5db
 * @param dot:              whether include 0.5. for example Es8388SetAdcDacVolume(ES_MODULE_ADC, 30, 4); means set ADC volume -30db
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
static int Es8388SetAdcDacVolume(int mode, int volume, int dot)
{
    int res = 0;
    if ( volume < -96 || volume > 0 ) {
        LOG_8388("Warning: volume < -96! or > 0!\n");
        if (volume < -96)
            volume = -96;
        else
            volume = 0;
    }
    dot = (dot >= 5 ? 1 : 0);
    volume = (-volume << 1) + dot;
    if (mode == ES_MODULE_ADC || mode == ES_MODULE_ADC_DAC) {
        res |= Es8388WriteReg(ES8388_ADDR, ES8388_ADCCONTROL8, volume);
        res |= Es8388WriteReg(ES8388_ADDR, ES8388_ADCCONTROL9, volume);  //ADC Right Volume=0db
    }
    if (mode == ES_MODULE_DAC || mode == ES_MODULE_ADC_DAC) {
        res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL5, volume);
        res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL4, volume);
    }
    return res;
}

/**
 * @brief Power Management
 *
 * @param mod:      if ES_POWER_CHIP, the whole chip including ADC and DAC is enabled
 * @param enable:   false to disable true to enable
 *
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
int Es8388Start(ESCodecModule mode)
{
    int res = 0;
    uint8_t prevData = 0, data = 0;
    Es8388ReadReg(ES8388_DACCONTROL21, &prevData);
    if (mode == ES_MODULE_LINE) {
        res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL16, 0x09); // 0x00 audio on LIN1&RIN1,  0x09 LIN2&RIN2 by pass enable
        res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL17, 0x50); // left DAC to left mixer enable  and  LIN signal to left mixer enable 0db  : bupass enable
        res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL20, 0x50); // right DAC to right mixer enable  and  LIN signal to right mixer enable 0db : bupass enable
        res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL21, 0xC0); //enable adc
    } else {
        res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL21, 0x80);   //enable dac
    }
    Es8388ReadReg(ES8388_DACCONTROL21, &data);
    if (prevData != data) {
        res |= Es8388WriteReg(ES8388_ADDR, ES8388_CHIPPOWER, 0xF0);   //start state machine
        // res |= Es8388WriteReg(ES8388_ADDR, ES8388_CONTROL1, 0x16);
        // res |= Es8388WriteReg(ES8388_ADDR, ES8388_CONTROL2, 0x50);
        res |= Es8388WriteReg(ES8388_ADDR, ES8388_CHIPPOWER, 0x00);   //start state machine
    }
    if (mode == ES_MODULE_ADC || mode == ES_MODULE_ADC_DAC || mode == ES_MODULE_LINE)
        res |= Es8388WriteReg(ES8388_ADDR, ES8388_ADCPOWER, 0x00);   //power up adc and line in
    //res |= Es8388SetAdcDacVolume(ES_MODULE_ADC, 0, 0);      // 0db
    if (mode == ES_MODULE_DAC || mode == ES_MODULE_ADC_DAC || mode == ES_MODULE_LINE) {
        res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACPOWER, 0x3c);   //power up dac and line out
        res |= Es8388SetVoiceMute(false);
        //res |= Es8388SetAdcDacVolume(ES_MODULE_DAC, 0, 0);      // 0db
    }

    return res;
}
/**
 * @brief Power Management
 *
 * @param mod:      if ES_POWER_CHIP, the whole chip including ADC and DAC is enabled
 * @param enable:   false to disable true to enable
 *
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
int Es8388Stop(ESCodecModule mode)
{
    int res = 0;
    if (mode == ES_MODULE_LINE) {
        res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL21, 0x80); //enable dac
        res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL16, 0x00); // 0x00 audio on LIN1&RIN1,  0x09 LIN2&RIN2
        res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL17, 0x90); // only left DAC to left mixer enable 0db
        res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL20, 0x90); // only right DAC to right mixer enable 0db
        return res;
    }
    if (mode == ES_MODULE_DAC || mode == ES_MODULE_ADC_DAC) {
        res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACPOWER, 0x00);
        res |= Es8388SetVoiceMute(true);
        //res |= Es8388SetAdcDacVolume(ES_MODULE_DAC, -96, 5);      // 0db
        //res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACPOWER, 0xC0);  //power down dac and line out
    }
    if (mode == ES_MODULE_ADC || mode == ES_MODULE_ADC_DAC) {
        //res |= Es8388SetAdcDacVolume(ES_MODULE_ADC, -96, 5);      // 0db
        res |= Es8388WriteReg(ES8388_ADDR, ES8388_ADCPOWER, 0xFF);  //power down adc and line in
    }
    if (mode == ES_MODULE_ADC_DAC) {
        res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL21, 0x9C);  //disable mclk
//        res |= Es8388WriteReg(ES8388_ADDR, ES8388_CONTROL1, 0x00);
//        res |= Es8388WriteReg(ES8388_ADDR, ES8388_CONTROL2, 0x58);
//        res |= Es8388WriteReg(ES8388_ADDR, ES8388_CHIPPOWER, 0xF3);  //stop state machine
    }

    return res;
}


/**
 * @brief Config I2s clock in MSATER mode
 *
 * @param cfg.sclkDiv:      generate SCLK by dividing MCLK in MSATER mode
 * @param cfg.lclkDiv:      generate LCLK by dividing MCLK in MSATER mode
 *
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
int Es8388I2sConfigClock(ESCodecI2sClock cfg)
{
    int res = 0;
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_MASTERMODE, cfg.sclkDiv);
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_ADCCONTROL5, cfg.lclkDiv);  //ADCFsMode,singel SPEED,RATIO=256
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL2, cfg.lclkDiv);  //ADCFsMode,singel SPEED,RATIO=256
    return res;
}

void Es8388Uninit()
{
    Es8388WriteReg(ES8388_ADDR, ES8388_CHIPPOWER, 0xFF);  //reset and stop es8388
    // i2c_driver_delete(cfg->i2c_port_num);
}

/**
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
int Es8388Init(CodecConfig *cfg)
{
    int res = 0;
    res = I2cInit(&cfg->i2c_cfg, cfg->i2c_port_num); // ESP32 in master mode
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL3, 0x04);  // 0x04 mute/0x00 unmute&ramp;DAC unmute and  disabled digital volume control soft ramp
    /* Chip Control and Power Management */
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_CONTROL2, 0x50);
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_CHIPPOWER, 0x00); //normal all and power up all
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_MASTERMODE, cfg->esMode); //CODEC IN I2S SLAVE MODE

    /* dac */
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACPOWER, 0xC0);  //disable DAC and disable Lout/Rout/1/2
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_CONTROL1, 0x12);  //Enfr=0,Play&Record Mode,(0x17-both of mic&paly)
//    res |= Es8388WriteReg(ES8388_ADDR, ES8388_CONTROL2, 0);  //LPVrefBuf=0,Pdn_ana=0
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL1, 0x18);//1a 0x18:16bit iis , 0x00:24
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL2, 0x02);  //DACFsMode,SINGLE SPEED; DACFsRatio,256
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL16, 0x00); // 0x00 audio on LIN1&RIN1,  0x09 LIN2&RIN2
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL17, 0x90); // only left DAC to left mixer enable 0db
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL20, 0x90); // only right DAC to right mixer enable 0db
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL21, 0x80); //set internal ADC and DAC use the same LRCK clock, ADC LRCK as internal LRCK
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL23, 0x00);   //vroi=0
    res |= Es8388SetAdcDacVolume(ES_MODULE_DAC, 0, 0);          // 0db
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACPOWER, cfg->dacOutput);  //0x3c Enable DAC and Enable Lout/Rout/1/2
    /* adc */
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_ADCPOWER, 0xFF);
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_ADCCONTROL1, 0x88); //0x88 MIC PGA =24DB
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_ADCCONTROL2, cfg->adcInput);  //0x00 LINSEL & RINSEL, LIN1/RIN1 as ADC Input; DSSEL,use one DS Reg11; DSR, LINPUT1-RINPUT1
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_ADCCONTROL3, 0x02);
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_ADCCONTROL4, 0x0c); //0d 0x0c I2S-16BIT, LEFT ADC DATA = LIN1 , RIGHT ADC DATA =RIN1
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_ADCCONTROL5, 0x02);  //ADCFsMode,singel SPEED,RATIO=256
    //ALC for Microphone
    res |= Es8388SetAdcDacVolume(ES_MODULE_ADC, 0, 0);      // 0db
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_ADCPOWER, 0x09); //Power up ADC, Enable LIN&RIN, Power down MICBIAS, set int1lp to low power mode
    /* stop all */
//    Es8388Stop(ES_MODULE_ADC_DAC);
    return res;
}

/**
 * @brief Configure ES8388 I2S format
 *
 * @param mode:           set ADC or DAC or all
 * @param bitPerSample:   see Es8388I2sFmt
 *
 * @return
 *     - (-1) Error
 *     - (0)  Success
 */
int Es8388ConfigFmt(ESCodecModule mode, ESCodecI2SFmt fmt)
{
    int res = 0;
    uint8_t reg = 0;
    if (mode == ES_MODULE_ADC || mode == ES_MODULE_ADC_DAC) {
        res = Es8388ReadReg(ES8388_ADCCONTROL4, &reg);
        reg = reg & 0xfc;
        res |= Es8388WriteReg(ES8388_ADDR, ES8388_ADCCONTROL4, reg | fmt);
    }
    if (mode == ES_MODULE_DAC || mode == ES_MODULE_ADC_DAC) {
        res = Es8388ReadReg(ES8388_DACCONTROL1, &reg);
        reg = reg & 0xf9;
        res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL1, reg | (fmt << 1));
    }
    return res;
}

/**
 * @param volume: 0 ~ 100
 *
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
int Es8388SetVoiceVolume(int volume)
{
    int res;
    if (volume < 0)
        volume = 0;
    else if (volume > 100)
        volume = 100;
    volume /= 3;
    res = Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL24, volume);
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL25, volume);  //ADC Right Volume=0db
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL26, 0);
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL27, 0);
    return res;
}
/**
 *
 * @return
 *           volume
 */
int Es8388GetVoiceVolume(int *volume)
{
    int res;
    uint8_t reg = 0;
    res = Es8388ReadReg(ES8388_DACCONTROL24, &reg);
    if (res == ESP_FAIL) {
        *volume = 0;
    } else {
        *volume = reg;
        *volume *= 3;
        if (*volume == 99)
            *volume = 100;
    }
    return res;
}

/**
 * @brief Configure ES8388 data sample bits
 *
 * @param mode:             set ADC or DAC or all
 * @param bitPerSample:   see BitsLength
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int Es8388SetBitsPerSample(ESCodecModule mode, BitsLength bitPerSample)
{
    int res = 0;
    uint8_t reg = 0;
    int bits = (int)bitPerSample;

    if (mode == ES_MODULE_ADC || mode == ES_MODULE_ADC_DAC) {
        res = Es8388ReadReg(ES8388_ADCCONTROL4, &reg);
        reg = reg & 0xe3;
        res |=  Es8388WriteReg(ES8388_ADDR, ES8388_ADCCONTROL4, reg | (bits << 2));
    }
    if (mode == ES_MODULE_DAC || mode == ES_MODULE_ADC_DAC) {
        res = Es8388ReadReg(ES8388_DACCONTROL1, &reg);
        reg = reg & 0xc7;
        res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL1, reg | (bits << 3));
    }
    return res;
}

/**
 * @brief Configure ES8388 DAC mute or not. Basicly you can use this function to mute the output or don't
 *
 * @param enable: enable or disable
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int Es8388SetVoiceMute(int enable)
{
    int res;
    uint8_t reg = 0;
    res = Es8388ReadReg(ES8388_DACCONTROL3, &reg);
    reg = reg & 0xFB;
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACCONTROL3, reg | (((int)enable) << 2));
    return res;
}

int Es8388GetVoiceMute(int *mute)
{
    int res = -1;
    uint8_t reg = 0;
    res = Es8388ReadReg(ES8388_DACCONTROL3, &reg);
    if (res == ESP_OK) {
        reg = (reg & 0x04) >> 2;
    }
    *mute = reg;
    return res;
}

/**
 * @param gain: Config DAC Output
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int EsConfigDacOutput(int output)
{
    int res;
    uint8_t reg = 0;
    res = Es8388ReadReg(ES8388_DACPOWER, &reg);
    reg = reg & 0xc3;
    res |= Es8388WriteReg(ES8388_ADDR, ES8388_DACPOWER, reg | output);
    return res;
}

/**
 * @param gain: Config ADC input
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int Es8388ConfigAdcInput(AdcInput input)
{
    return 0;
}

/**
 * @param gain: see MicGain
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int Es8388SetMicGain(MicGain gain)
{
    int res, gain_n;
    gain_n = (int)gain / 3;
    res = Es8388WriteReg(ES8388_ADDR, ES8388_ADCCONTROL1, gain_n); //MIC PGA
    return res;
}

void Es8388ReadAll()
{
    for (int i = 0; i < 50; i++) {
        uint8_t reg = 0;
        Es8388ReadReg(i, &reg);
        ets_printf("%x: %x\n", i, reg);
    }
}

int ES8388WriteReg(uint8_t regAdd, uint8_t data)
{
    return Es8388WriteReg(ES8388_ADDR, regAdd, data);
}
