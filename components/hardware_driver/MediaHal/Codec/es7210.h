/*
 * ALSA SoC ES7210 codec driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ES7210_H
#define _ES7210_H

#include "driver/i2c.h"

#define SAMPLE_RATE_16K_1
//#define SAMPLE_RATE_8K

//#define USE_ARM_IIC
#define CODEC_IIC  I2C_NODE0_BASE
#define ES7210_MCLK_KHZ             4096  //4.096Mhz

#define ES7210_RESET_CTL_REG00      0x00
#define ES7210_CLK_ON_OFF_REG01     0x01
#define ES7210_MCLK_CTL_REG02       0x02
#define ES7210_MST_CLK_CTL_REG03    0x03
#define ES7210_MST_LRCDIVH_REG04    0x04
#define ES7210_MST_LRCDIVL_REG05    0x05
#define ES7210_DIGITAL_PDN_REG06    0x06
#define ES7210_ADC_OSR_REG07        0x07
#define ES7210_MODE_CFG_REG08       0x08

#define ES7210_TCT0_CHPINI_REG09    0x09
#define ES7210_TCT1_CHPINI_REG0A    0x0A
#define ES7210_CHIP_STA_REG0B       0x0B

#define ES7210_IRQ_CTL_REG0C        0x0C
#define ES7210_MISC_CTL_REG0D       0x0D
#define ES7210_DMIC_CTL_REG10       0x10

#define ES7210_SDP_CFG1_REG11       0x11
#define ES7210_SDP_CFG2_REG12       0x12

#define ES7210_ADC_AUTOMUTE_REG13   0x13
#define ES7210_ADC34_MUTE_REG14     0x14
#define ES7210_ADC12_MUTE_REG15     0x15

#define ES7210_ALC_SEL_REG16        0x16
#define ES7210_ALC_COM_CFG1_REG17   0x17
#define ES7210_ALC34_LVL_REG18      0x18
#define ES7210_ALC12_LVL_REG19      0x19
#define ES7210_ALC_COM_CFG2_REG1A   0x1A
#define ES7210_ALC4_MAX_GAIN_REG1B  0x1B
#define ES7210_ALC3_MAX_GAIN_REG1C  0x1C
#define ES7210_ALC2_MAX_GAIN_REG1D  0x1D
#define ES7210_ALC1_MAX_GAIN_REG1E  0x1E

#define ES7210_ADC34_HPF2_REG20     0x20
#define ES7210_ADC34_HPF1_REG21     0x21
#define ES7210_ADC12_HPF2_REG22     0x22
#define ES7210_ADC12_HPF1_REG23     0x23

#define ES7210_CHP_ID1_REG3D        0x3D
#define ES7210_CHP_ID0_REG3E        0x3E
#define ES7210_CHP_VER_REG3F        0x3F

#define ES7210_ANALOG_SYS_REG40     0x40

#define ES7210_MICBIAS12_REG41      0x41
#define ES7210_MICBIAS34_REG42      0x42
#define ES7210_MIC1_GAIN_REG43      0x43
#define ES7210_MIC2_GAIN_REG44      0x44
#define ES7210_MIC3_GAIN_REG45      0x45
#define ES7210_MIC4_GAIN_REG46      0x46
#define ES7210_MIC1_LP_REG47        0x47
#define ES7210_MIC2_LP_REG48        0x48
#define ES7210_MIC3_LP_REG49        0x49
#define ES7210_MIC4_LP_REG4A        0x4A
#define ES7210_MIC12_PDN_REG4B      0x4B
#define ES7210_MIC34_PDN_REG4C      0x4C

///////////////////////////////////////////////////
#define ENABLE      1
#define DISABLE     0

#define MIC_CHN_16  16
#define MIC_CHN_14  14
#define MIC_CHN_12  12
#define MIC_CHN_10  10
#define MIC_CHN_8   8
#define MIC_CHN_6   6
#define MIC_CHN_4   4
#define MIC_CHN_2   2

#define ES7210_TDM_ENABLE   ENABLE
#define ES7210_CHANNELS_MAX MIC_CHN_4

#if ES7210_CHANNELS_MAX == MIC_CHN_2
    #define ADC_DEV_MAXNUM  1
#endif

#if ES7210_CHANNELS_MAX == MIC_CHN_4
    #define ADC_DEV_MAXNUM  1
#endif
#if ES7210_CHANNELS_MAX == MIC_CHN_6
    #define ADC_DEV_MAXNUM  2
#endif
#if ES7210_CHANNELS_MAX == MIC_CHN_8
    #define ADC_DEV_MAXNUM  2
#endif
#if ES7210_CHANNELS_MAX == MIC_CHN_10
    #define ADC_DEV_MAXNUM  3
#endif
#if ES7210_CHANNELS_MAX == MIC_CHN_12
    #define ADC_DEV_MAXNUM  3
#endif
#if ES7210_CHANNELS_MAX == MIC_CHN_14
    #define ADC_DEV_MAXNUM  4
#endif
#if ES7210_CHANNELS_MAX == MIC_CHN_16
    #define ADC_DEV_MAXNUM  4
#endif

#define ES7210_TDM_1LRCK_DSPA                 0
#define ES7210_TDM_1LRCK_DSPB                 1
#define ES7210_TDM_1LRCK_I2S                  2
#define ES7210_TDM_1LRCK_LJ                   3
#define ES7210_TDM_NLRCK_DSPA                 4
#define ES7210_TDM_NLRCK_DSPB                 5
#define ES7210_TDM_NLRCK_I2S                  6
#define ES7210_TDM_NLRCK_LJ                   7
#define ES7210_NORMAL_I2S                     8
#define ES7210_NORMAL_LJ                      9
#define ES7210_NORMAL_DSPA                    10
#define ES7210_NORMAL_DSPB                    11

#define SDP_16BIT_LENGTH                      0
#define SDP_18BIT_LENGTH                      1
#define SDP_20BIT_LENGTH                      2
#define SDP_24BIT_LENGTH                      3
#define SDP_32BIT_LENGTH                      4

#define MASTER    0
#define SLAVE     1

#define ES7210_WORK_MODE_MASTER    ES7210_TDM_1LRCK_I2S | (MASTER  << 7) | (SDP_16BIT_LENGTH << 4)
#define ES7210_WORK_MODE_SLAVE     ES7210_TDM_1LRCK_I2S | (SLAVE  << 7) | (SDP_16BIT_LENGTH << 4)

#define CPU_I2S_MODE        SLAVE

///////////////////////////////////////////////////////////////////////
/* codec private data */
struct es7210_priv
{
    //unsigned int adc_num;  // the number of ES7210 in tdm link

    unsigned int mclk;     //the frequency of MCLK clock
    unsigned int lrck;       // the frequency of LRCK clock

    unsigned char i2c_chip_id[ADC_DEV_MAXNUM];  // i2c address of es7210 devices
    unsigned char tdm_mode[ADC_DEV_MAXNUM];  //tdm_mode[0] for adc0, tdm_mode[1] for adc1
                                                                                    //tdm_mode[2] for adc2, tdm_mode[3] for adc3
} ;
extern struct es7210_priv es7210_private;

int Es7210Init(void);
int es7210_read(unsigned char reg, unsigned char *rt_value, unsigned char i2c_chip_addr);

// static int __attribute__((unused)) es7210_mute(unsigned char mute);

#endif /* _ES7210_H_ */
