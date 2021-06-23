/*
 * ALSA SoC ES7210 adc driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Notes:
 *  ES7210 is a 4-ch ADC of Everest
 *
 *
 */

#include "es7210.h"
#include "esp_system.h"
#include "esp_types.h"
#include "driver/i2c.h"
#include "esp_log.h"

#define ES7210_TAG "7210"

#define ES_ASSERT(a, format, b, ...) \
    if ((a) != 0) { \
        ESP_LOGE(ES7210_TAG, format, ##__VA_ARGS__); \
        return b;\
    }


// #define SAMPLE_RATE_16K
//#define SAMPLE_RATE_8K



struct es7210_priv es7210_private;

struct es7210_reg_config
{
    unsigned char reg_addr;
    unsigned char reg_v;
};
static const struct es7210_reg_config es7210_tdm_reg_common_cfg1[] =
{
    { 0x00, 0xFF },
    { 0x00, 0x30 },
    { 0x01, 0x21 },   //for wangyebin ,disable mclk
    { 0x09, 0x30 },
    { 0x0A, 0x30 },
    { 0x23, 0x2A },
    { 0x22, 0x0A },
    { 0x21, 0x2A },
    { 0x20, 0x0A },
};
//static const struct es7210_reg_config es7210_tdm_reg_fmt_cfg[] =
//{
//    { 0x11, 0x63 },
//    { 0x12, 0x01 },
//};

// static const struct es7210_reg_config es7210_tdm_reg_common_cfg2[] =
// {
//     { 0x40, 0xC3 },
//     { 0x41, 0x70 },
//     { 0x42, 0x70 },
//     { 0x43, 0x1B },
//     { 0x44, 0x1B },
//     { 0x45, 0x1B },
//     { 0x46, 0x18 },
//     { 0x47, 0x08 },
//     { 0x48, 0x08 },
//     { 0x49, 0x08 },
//     { 0x4A, 0x08 },
//     { 0x07, 0x20 },
// };


// static const struct es7210_reg_config es7210_tdm_reg_common_cfg2[] =
// {
//     { 0x40, 0xC3 },
//     { 0x41, 0x70 },
//     { 0x42, 0x70 },
//     { 0x43, 0x1E },
//     { 0x44, 0x1E },
//     { 0x45, 0x1E },
//     { 0x46, 0x1E },
//     { 0x47, 0x08 },
//     { 0x48, 0x08 },
//     { 0x49, 0x08 },
//     { 0x4A, 0x08 },
//     { 0x07, 0x20 },
// };

// korvo-mix
static const struct es7210_reg_config es7210_tdm_reg_common_cfg2[] =
{
    { 0x40, 0xC3 },
    { 0x41, 0x70 },
    { 0x42, 0x70 },
    { 0x43, 0x1E },
    { 0x44, 0x1E },
    { 0x45, 0x18 },
    { 0x46, 0x1E },
    { 0x47, 0x08 },
    { 0x48, 0x08 },
    { 0x49, 0x08 },
    { 0x4A, 0x08 },
    { 0x07, 0x20 },
};


//static const struct es7210_reg_config es7210_tdm_reg_mclk_cfg[] =
//{
//    { 0x02, 0xC3 },
//};
static const struct es7210_reg_config es7210_tdm_reg_common_cfg3[] =
{
    { 0x06, 0x04 },
    { 0x4B, 0x0F },
    { 0x4C, 0x0F },
    { 0x00, 0x71 },
    { 0x00, 0x41 },

#ifdef SAMPLE_RATE_8K
    /////8k sample rate
    { 0x03, 0x0c },
    { 0x04, 0x06 },
    { 0x05, 0x00 },
#endif

#ifdef SAMPLE_RATE_16K_1
    /////16k sample rate
    { 0x03, 0x06 },
    { 0x04, 0x03 },
    { 0x05, 0x00 },
#endif
};
/*
*  es7210_read(), read the register of es7210
*
*    unsigned int reg, the register address
*  unsigned int *rt_value, the value of register
*    unsigned int i2c_chip_addr, the i2c chip address of es7210 device
*
*  return
*               0, success, 1, fail
*/
int es7210_read(unsigned char reg, unsigned char *rt_value, unsigned char i2c_chip_addr)
{
    uint8_t data;
    int res = 0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    res |= i2c_master_start(cmd);
    res |= i2c_master_write_byte(cmd, i2c_chip_addr, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_write_byte(cmd, reg, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_stop(cmd);
    res |= i2c_master_cmd_begin(0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    cmd = i2c_cmd_link_create();
    res |= i2c_master_start(cmd);
    res |= i2c_master_write_byte(cmd, i2c_chip_addr | 0x01, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_read_byte(cmd, &data, 0x01/*NACK_VAL*/);
    res |= i2c_master_stop(cmd);
    res |= i2c_master_cmd_begin(0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    ES_ASSERT(res, "Es7210ReadReg error", -1);
    *rt_value = data;
    return res;
}
/*
*  es7210_write(), write the register of es7210
*
*    unsigned int reg, the register address
*  unsigned int value, the value of register
*    unsigned int i2c_chip_addr, the i2c chip address of es7210 device
*
*  return
*               0, success, 1, fail
*/
// static int es7210_write(unsigned char reg, unsigned char value, unsigned char i2c_chip_addr)
// {
//  int ret = 0;

//  //ret = i2c_master_send(i2c_chip_addr, reg, value);
//  printf("WYB:ES7210 ID= %x,REG 0x%x = 0x%x,\n",i2c_chip_addr,reg,value);
// #ifdef USE_ARM_IIC
//  hwIIC_Write(i2c_chip_addr, reg, value);
// #else
//  hwIIC_DSP_Write((SB35PS_I2C)(CODEC_IIC),i2c_chip_addr, reg,value);
// #endif
//  if (ret != 0)
//  {
//      printf("es7210_write error->[REG-0x%02x,val-0x%02x]\n",reg,value);
//      return 1;
//  }

//  return 0;
// }

static int es7210_write(unsigned char reg, unsigned char value, unsigned char i2c_chip_addr)
{
    int res = 0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    res |= i2c_master_start(cmd);
    res |= i2c_master_write_byte(cmd, i2c_chip_addr, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_write_byte(cmd, reg, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_write_byte(cmd, value, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_stop(cmd);
    res |= i2c_master_cmd_begin(0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    // ES_ASSERT(res, "ESCodecWriteReg error", -1);
    return res;
}









/*
*  es7210_update_bits(), update the specific bit of register
*
*    unsigned int reg, the register address
*  unsigned int mask, the mask of bitmap
*  unsigned int value, the value of register
*    unsigned int i2c_chip_addr, the i2c chip address of es7210 device
*
*  return
*               0, success, 1, fail
*/
static int es7210_update_bits(unsigned char reg, unsigned char mask, unsigned char value, unsigned char i2c_chip_addr)
{
    unsigned char val_old,val_new;

    es7210_read(reg, &val_old, i2c_chip_addr);
    val_new = (val_old & ~mask) | (value & mask);
    if(val_new != val_old)
    {
        es7210_write(reg, val_new, i2c_chip_addr);
    }

    return 0;
}

/*
*  es7210_multi_chips_write(), write the same of register address of multiple es7210 chips
*
*    unsigned int reg, the register address
*  unsigned int value, the value of register
*
*  return
*               0, success, 1, fail
*/
static int es7210_multi_chips_write(unsigned char reg, unsigned char value)
{
    unsigned char i;

    for(i=0; i< ADC_DEV_MAXNUM; i++)
    {
        es7210_write(reg, value, es7210_private.i2c_chip_id[i]);
    }
    return 0;
}
/*
*  es7210_multi_chips_update_bits(), update the same of register bits of multiple es7210 chips
*
*    unsigned int reg, the register address
*  unsigned int value, the value of register
*
*  return
*               0, success, 1, fail
*/
static int es7210_multi_chips_update_bits(unsigned char reg, unsigned char mask, unsigned char value)
{
    unsigned char i;

    for(i=0; i< ADC_DEV_MAXNUM; i++)
    {
        es7210_update_bits(reg, mask, value, es7210_private.i2c_chip_id[i]);
    }

    return 0;
}
/*
*  es7210_mute(), set es7210 in mute or unmute
*
*    unsigned int mute, = 1, mute
*                                           = 0, unmute
*  return
*               0, success, 1, fail
*/
static int __attribute__((unused)) es7210_mute(unsigned char mute)
{

    if (mute)
    {
        es7210_multi_chips_update_bits(ES7210_ADC34_MUTE_REG14, 0x03, 0x03);
        es7210_multi_chips_update_bits(ES7210_ADC12_MUTE_REG15, 0x03, 0x03);
    }
    else
    {
        es7210_multi_chips_update_bits(ES7210_ADC34_MUTE_REG14, 0x03, 0x00);
        es7210_multi_chips_update_bits(ES7210_ADC12_MUTE_REG15, 0x03, 0x00);
    }
    return 0;
}
/*
*  es7210_micboost1_setting_set(),    to set the mic gain for mic1
*  es7210_micboost2_setting_set(),    to set the mic gain for mic2
*  es7210_micboost3_setting_set(),    to set the mic gain for mic3
*  es7210_micboost4_setting_set(),    to set the mic gain for mic4
*  es7210_micboost5_setting_set(),    to set the mic gain for mic5
*  es7210_micboost6_setting_set(),    to set the mic gain for mic6
*  es7210_micboost7_setting_set(),    to set the mic gain for mic7
*  es7210_micboost8_setting_set(),    to set the mic gain for mic8
*  es7210_micboost9_setting_set(),    to set the mic gain for mic9
*  es7210_micboost10_setting_set(),    to set the mic gain for mic10
*  es7210_micboost11_setting_set(),    to set the mic gain for mic11
*  es7210_micboost12_setting_set(),    to set the mic gain for mic12
*  es7210_micboost13_setting_set(),    to set the mic gain for mic13
*  es7210_micboost14_setting_set(),    to set the mic gain for mic14
*  es7210_micboost15_setting_set(),    to set the mic gain for mic15
*  es7210_micboost16_setting_set(),    to set the mic gain for mic16
*
*    unsigned int gain, the pag gain setting
*                                           = 0, 0db
*                                           = 1, 3db
*                                           = 2, 6db
*                     = 3, 9db
*                     .........
*                                           = 10, 30db
*                     = 11, 33db
*                     = 12, 34.5db
*                     = 13, 36db
*                                           = 14, 37.5db
*  return
*               0, success, 1, fail
*/
#if ES7210_CHANNELS_MAX > 0
static int __attribute__((unused)) es7210_micboost1_setting_set(unsigned char gain)
{
    if(gain > 14 )
        gain = 14;
    es7210_update_bits(0x43, 0x0F, gain, es7210_private.i2c_chip_id[0]);
    return 0;
}

static int __attribute__((unused)) es7210_micboost2_setting_set(unsigned char gain)
{
    if(gain > 14 )
        gain = 14;
    es7210_update_bits(0x44, 0x0F, gain, es7210_private.i2c_chip_id[0]);
    return 0;
}

static int __attribute__((unused)) es7210_micboost3_setting_set(unsigned char gain)
{
    if(gain > 14 )
        gain = 14;
    es7210_update_bits(0x45, 0x0F, gain, es7210_private.i2c_chip_id[0]);
    return 0;
}

static int __attribute__((unused)) es7210_micboost4_setting_set(unsigned char gain)
{
    if(gain > 14 )
        gain = 14;
    es7210_update_bits(0x46, 0x0F, gain, es7210_private.i2c_chip_id[0]);
    return 0;
}
#endif

#if ES7210_CHANNELS_MAX > 4
static int es7210_micboost5_setting_set(unsigned char gain)
{
    if(gain > 14 )
        gain = 14;
    es7210_update_bits(0x43, 0x0F, gain, es7210_private.i2c_chip_id[1]);
    return 0;
}

static int es7210_micboost6_setting_set(unsigned char gain)
{
    if(gain > 14 )
        gain = 14;
    es7210_update_bits(0x44, 0x0F, gain, es7210_private.i2c_chip_id[1]);
    return 0;
}

static int es7210_micboost7_setting_set(unsigned char gain)
{
    if(gain > 14 )
        gain = 14;
    es7210_update_bits(0x45, 0x0F, gain, es7210_private.i2c_chip_id[1]);
    return 0;
}

static int es7210_micboost8_setting_set(unsigned char gain)
{
    if(gain > 14 )
        gain = 14;
    es7210_update_bits(0x46, 0x0F, gain, es7210_private.i2c_chip_id[1]);
    return 0;
}
#endif

#if ES7210_CHANNELS_MAX > 8
static int es7210_micboost9_setting_set(unsigned char gain)
{
    if(gain > 14 )
        gain = 14;
    es7210_update_bits(0x43, 0x0F, gain, es7210_private.i2c_chip_id[2]);
    return 0;
}

static int es7210_micboost10_setting_set(unsigned char gain)
{
    if(gain > 14 )
        gain = 14;
    es7210_update_bits(0x44, 0x0F, gain, es7210_private.i2c_chip_id[2]);
    return 0;
}

static int es7210_micboost11_setting_set(unsigned char gain)
{
    if(gain > 14 )
        gain = 14;
    es7210_update_bits(0x45, 0x0F, gain, es7210_private.i2c_chip_id[2]);
    return 0;
}

static int es7210_micboost12_setting_set(unsigned char gain)
{
    if(gain > 14 )
        gain = 14;
    es7210_update_bits(0x46, 0x0F, gain, es7210_private.i2c_chip_id[2]);
    return 0;
}
#endif
#if ES7210_CHANNELS_MAX > 12
static int es7210_micboost13_setting_set(unsigned char gain)
{
    if(gain > 14 )
        gain = 14;
    es7210_update_bits(0x43, 0x0F, gain, es7210_private.i2c_chip_id[3]);
    return 0;
}

static int es7210_micboost14_setting_set(unsigned char gain)
{
    if(gain > 14 )
        gain = 14;
    es7210_update_bits(0x44, 0x0F, gain, es7210_private.i2c_chip_id[3]);
    return 0;
}

static int es7210_micboost15_setting_set(unsigned char gain)
{
    if(gain > 14 )
        gain = 14;
    es7210_update_bits(0x45, 0x0F, gain, es7210_private.i2c_chip_id[3]);
    return 0;
}

static int es7210_micboost16_setting_set(unsigned char gain)
{
    if(gain > 14 )
        gain = 14;
    es7210_update_bits(0x46, 0x0F, gain, es7210_private.i2c_chip_id[3]);
    return 0;
}
#endif

/*
* es7210_tdm_init_codec(), to initialize es7210 for tdm mode
*
*/
static void es7210_init_codec(void)
{
    int cnt, channel;
    unsigned int mode, len;

    for(cnt = 0; cnt < sizeof(es7210_tdm_reg_common_cfg1)/sizeof(es7210_tdm_reg_common_cfg1[0]); cnt++)
    {
        es7210_multi_chips_write(es7210_tdm_reg_common_cfg1[cnt].reg_addr,
                        es7210_tdm_reg_common_cfg1[cnt].reg_v);
    }
    /*
    *   set ES7210 in master or slave mode
    */
    for(cnt = 0; cnt < ADC_DEV_MAXNUM; cnt++)
    {
        if(es7210_private.tdm_mode[cnt] & 0x80)   //slave mode
        {
            es7210_write(ES7210_MODE_CFG_REG08, 0x10, es7210_private.i2c_chip_id[cnt]);
            // es7210_write(ES7210_DMIC_CTL_REG10, 0xc9, es7210_private.i2c_chip_id[cnt]);

            /* Following are the controllers of mics gain, can be set according to ES7210 datasheet p.23 */
            // es7210_write(ES7210_MIC1_GAIN_REG43, 0x15, es7210_private.i2c_chip_id[cnt]);
            // es7210_write(ES7210_MIC2_GAIN_REG44, 0x15, es7210_private.i2c_chip_id[cnt]);
            // es7210_write(ES7210_MIC3_GAIN_REG45, 0x15, es7210_private.i2c_chip_id[cnt]);
            // es7210_write(ES7210_MIC4_GAIN_REG46, 0x15, es7210_private.i2c_chip_id[cnt]);

            // printf("WYB:Slave mode:%x !\n",es7210_private.i2c_chip_id[cnt]);
        }
        else                                      //master mode
        {
            es7210_write(ES7210_MODE_CFG_REG08, 0x11, es7210_private.i2c_chip_id[cnt]);
            // printf("WYB:Master mode:%x !\n",es7210_private.i2c_chip_id[cnt]);
        }
    }

    mode = es7210_private.tdm_mode[0] & 0x0f;
    switch(mode)
    {
        case ES7210_TDM_1LRCK_DSPA:
            /*
            * Here to set TDM format for DSP-A mode
            */
            es7210_multi_chips_update_bits(ES7210_SDP_CFG1_REG11, 0x03, 0x03);
            es7210_multi_chips_write(ES7210_SDP_CFG2_REG12, 0x01);
            break;
      case ES7210_TDM_1LRCK_DSPB:
            /*
            * Here to set TDM format for DSP-B mode
            */
            es7210_multi_chips_update_bits(ES7210_SDP_CFG1_REG11, 0x013, 0x13);
            es7210_multi_chips_write(ES7210_SDP_CFG2_REG12, 0x01);
            break;
      case ES7210_TDM_1LRCK_I2S:
            /*
            * Here to set TDM format for I2S mode
            */
            es7210_multi_chips_update_bits(ES7210_SDP_CFG1_REG11, 0x03, 0x00);
            es7210_multi_chips_write(ES7210_SDP_CFG2_REG12, 0x02);
            break;
      case ES7210_TDM_1LRCK_LJ:
            /*
            * Here to set TDM format for Left Justified mode
            */
            es7210_multi_chips_update_bits(ES7210_SDP_CFG1_REG11, 0x03, 0x01);
            es7210_multi_chips_write(ES7210_SDP_CFG2_REG12, 0x02);
            break;
      case ES7210_TDM_NLRCK_DSPA:
            /*
             * Here to set TDM format for DSP-A with multiple LRCK TDM mode
             */
            channel = ES7210_CHANNELS_MAX;
            /*
            * Set the microphone numbers in array
            */
            switch(channel)
            {
                case 2:
                    es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x10);
                    break;
                case 4:
                    es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x20);
                    break;
                case 6:
                    es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x30);
                    break;
                case 8:
                    es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x40);
                    break;
                case 10:
                    es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x50);
                    break;
                case 12:
                    es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x60);
                    break;
                case 14:
                    es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x70);
                    break;
                case 16:
                    es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x80);
                    break;
                default:
                    break;
            }
            /*
            * set format, dsp-a with multiple LRCK tdm mode
            */
            es7210_multi_chips_update_bits(ES7210_SDP_CFG1_REG11, 0x03, 0x03);

            for(cnt = 0; cnt < ADC_DEV_MAXNUM; cnt++)
            {
                if(cnt == 0)
                {
                    /*
                    * set tdm flag in the interface chip
                    */
                    es7210_write(ES7210_SDP_CFG2_REG12, 0x07, es7210_private.i2c_chip_id[cnt]);
                }
                else
                {
                    es7210_write(ES7210_SDP_CFG2_REG12, 0x03, es7210_private.i2c_chip_id[cnt]);
                }
            }
            break;
      case ES7210_TDM_NLRCK_DSPB:
          /*
          * Here to set TDM format for DSP-B with multiple LRCK TDM mode
          */
          channel = ES7210_CHANNELS_MAX;
          /*
          * Set the microphone numbers in array
          */
          switch(channel)
          {
              case 2:
                  es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x10);
                  break;
              case 4:
                  es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x20);
                  break;
              case 6:
                  es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x30);
                  break;
              case 8:
                  es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x40);
                  break;
              case 10:
                  es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x50);
                  break;
              case 12:
                  es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x60);
                  break;
              case 14:
                  es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x70);
                  break;
              case 16:
                  es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x80);
                  break;
              default:
                  break;
           }
          /*
          * set format, dsp-b with multiple LRCK tdm mode
          */
          es7210_multi_chips_update_bits(ES7210_SDP_CFG1_REG11, 0x13, 0x13);

          for(cnt = 0; cnt < ADC_DEV_MAXNUM; cnt++)
          {
              if(cnt == 0)
              {
                  /*
                   * set tdm flag in the interface chip
                   */
                  es7210_write(ES7210_SDP_CFG2_REG12, 0x07, es7210_private.i2c_chip_id[cnt]);
              }
              else
              {
                  es7210_write(ES7210_SDP_CFG2_REG12, 0x03, es7210_private.i2c_chip_id[cnt]);
              }
          }
          break;
      case ES7210_TDM_NLRCK_I2S:
          /*
          * Here to set TDM format for I2S with multiple LRCK TDM mode
          */
          channel = ES7210_CHANNELS_MAX;
          /*
          * Set the microphone numbers in array
          */
          switch(channel)
          {
              case 2:
                      es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x10);
                      break;
              case 4:
                      es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x20);
                      break;
              case 6:
                      es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x30);
                      break;
              case 8:
                      es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x40);
                      break;
              case 10:
                      es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x50);
                      break;
              case 12:
                      es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x60);
                      break;
              case 14:
                      es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x70);
                      break;
              case 16:
                      es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x80);
                      break;
              default:
                      break;
          }
          /*
          * set format, I2S with multiple LRCK tdm mode
          */
          es7210_multi_chips_update_bits(ES7210_SDP_CFG1_REG11, 0x03, 0x00);

          for(cnt = 0; cnt < ADC_DEV_MAXNUM; cnt++)
          {
              if(cnt == 0)
              {
                  /*
                  * set tdm flag in the interface chip
                  */
                  es7210_write(ES7210_SDP_CFG2_REG12, 0x07, es7210_private.i2c_chip_id[cnt]);
              }
              else
              {
                  es7210_write(ES7210_SDP_CFG2_REG12, 0x03, es7210_private.i2c_chip_id[cnt]);
              }
          }

          break;
        case ES7210_TDM_NLRCK_LJ:
            /*
            * Here to set TDM format for left justified with multiple LRCK TDM mode
            */
            channel = ES7210_CHANNELS_MAX;
            /*
            * Set the microphone numbers in array
            */
           switch(channel)
           {
                case 2:
                        es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x10);
                        break;
                case 4:
                        es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x20);
                        break;
                case 6:
                        es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x30);
                        break;
                case 8:
                        es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x40);
                        break;
                case 10:
                        es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x50);
                        break;
                case 12:
                        es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x60);
                        break;
                case 14:
                        es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x70);
                        break;
                case 16:
                        es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0xf0, 0x80);
                        break;
                default:
                        break;
           }
            /*
            * set format, left justified with multiple LRCK tdm mode
            */
            es7210_multi_chips_update_bits(ES7210_SDP_CFG1_REG11, 0x03, 0x01);

            for(cnt = 0; cnt < ADC_DEV_MAXNUM; cnt++)
            {
                if(cnt == 0)
                {
                    /*
                     * set tdm flag in the interface chip
                     */
                    es7210_write(ES7210_SDP_CFG2_REG12, 0x07, es7210_private.i2c_chip_id[cnt]);
                }
                else
                {
                    es7210_write(ES7210_SDP_CFG2_REG12, 0x03, es7210_private.i2c_chip_id[cnt]);
                }
            }

            break;
         case ES7210_NORMAL_I2S:
            /*
            * here to disable tdm and set i2s-16bit for normal mode
            */
            es7210_multi_chips_update_bits(ES7210_SDP_CFG1_REG11, 0x03, 0x00); //i2s
            es7210_multi_chips_write(ES7210_SDP_CFG2_REG12, 0x00); //disable tdm
            break;
        case ES7210_NORMAL_LJ:
            /*
            * here to disable tdm and set i2s-16bit for normal mode
            */
            es7210_multi_chips_update_bits(ES7210_SDP_CFG1_REG11, 0x03, 0x01); //lj
            es7210_multi_chips_write(ES7210_SDP_CFG2_REG12, 0x00); //disable tdm
            break;
        case ES7210_NORMAL_DSPA:
            /*
            * here to disable tdm and set i2s-16bit for normal mode
            */
            es7210_multi_chips_update_bits(ES7210_SDP_CFG1_REG11, 0x03, 0x03); //dsp-a
            es7210_multi_chips_write(ES7210_SDP_CFG2_REG12, 0x00); //disable tdm
            break;
        case ES7210_NORMAL_DSPB:
            /*
            * here to disable tdm and set i2s-16bit for normal mode
            */
            es7210_multi_chips_update_bits(ES7210_SDP_CFG1_REG11, 0x13, 0x13); //dsp-b
            es7210_multi_chips_write(ES7210_SDP_CFG2_REG12, 0x00); //disable tdm
            break;
    }

    /*
    *   set the data length
    */
    len = (es7210_private.tdm_mode[0] & 0x70) >> 4;
    switch(len)
    {
        case SDP_16BIT_LENGTH:
            es7210_multi_chips_update_bits(ES7210_SDP_CFG1_REG11, 0xe0, 0x60);
            break;
        case SDP_18BIT_LENGTH:
            es7210_multi_chips_update_bits(ES7210_SDP_CFG1_REG11, 0xe0, 0x40);
            break;
        case SDP_20BIT_LENGTH:
            es7210_multi_chips_update_bits(ES7210_SDP_CFG1_REG11, 0xe0, 0x20);
            break;
        case SDP_24BIT_LENGTH:
            es7210_multi_chips_update_bits(ES7210_SDP_CFG1_REG11, 0xe0, 0x00);
            break;
        case SDP_32BIT_LENGTH:
            es7210_multi_chips_update_bits(ES7210_SDP_CFG1_REG11, 0xe0, 0x80);
            break;
        default:
            es7210_multi_chips_update_bits(ES7210_SDP_CFG1_REG11, 0xe0, 0x60); // default for 16bit
            break;
    }

    for(cnt = 0; cnt < sizeof(es7210_tdm_reg_common_cfg2)/sizeof(es7210_tdm_reg_common_cfg2[0]); cnt++)
    {
        es7210_multi_chips_write(es7210_tdm_reg_common_cfg2[cnt].reg_addr,
                                            es7210_tdm_reg_common_cfg2[cnt].reg_v);
    }

    switch(mode)
    {
        case ES7210_TDM_1LRCK_DSPA:
        case ES7210_TDM_1LRCK_DSPB:
        case ES7210_TDM_1LRCK_I2S:
        case ES7210_TDM_1LRCK_LJ:
            /*
            * to set internal mclk
            * here, we assume that cpu/soc always provides 256FS i2s clock to es7210.
            * dll bypassed, use clock doubler to get double frequency for internal modem which need
            * 512FS clock. the clk divider ratio is 1.
            * user must modify the setting of register0x02 according to FS ratio provided by CPU/SOC.
            */
            // printf("WYB:ES7210_MCLK_CTL_REG02 = 0xC3!\n");
            es7210_multi_chips_write(ES7210_MCLK_CTL_REG02, 0xc1);
            break;
        case ES7210_TDM_NLRCK_DSPA:
        case ES7210_TDM_NLRCK_DSPB:
        case ES7210_TDM_NLRCK_I2S:
        case ES7210_TDM_NLRCK_LJ:
            /*
            * to set internal mclk
            * here, we assume that cpu/soc always provides 256FS i2s clock to es7210 and there is four
                    * es7210 devices in tdm link. so the internal FS in es7210 is only FS/4;
            * dll bypassed, clock doubler bypassed. the clk divider ratio is 2. so the clock of internal
                    * modem equals to (256FS / (FS/4) / 2) * FS = 512FS
            * user must modify the setting of register0x02 according to FS ratio provided by CPU/SOC.
            */
            // printf("WYB:ES7210_MCLK_CTL_REG02 = 0xC3!!\n");
            es7210_multi_chips_write(ES7210_MCLK_CTL_REG02, 0xc3);
            break;
        default:
            /*
            * to set internal mclk for normal mode
            * here, we assume that cpu/soc always provides 256FS i2s clock to es7210.
            * dll bypassed, use clock doubler to get double frequency for internal modem which need
            * 512FS clock. the clk divider ratio is 1.
            * user must modify the setting of register0x02 according to FS ratio provided by CPU/SOC.
            */
            // printf("WYB:ES7210_MCLK_CTL_REG02 = 0xC3!!!\n");
            es7210_multi_chips_write(ES7210_MCLK_CTL_REG02, 0xc3);
            break;
    }
#if 1
    for(cnt = 0; cnt < sizeof(es7210_tdm_reg_common_cfg3)/sizeof(es7210_tdm_reg_common_cfg3[0]); cnt++)
    {
        es7210_multi_chips_write(es7210_tdm_reg_common_cfg3[cnt].reg_addr,
                                           es7210_tdm_reg_common_cfg3[cnt].reg_v);
    }

    /*
    *   set SCLK Divider
    */
    //es7210_multi_chips_write(ES7210_MST_CLK_CTL_REG03,0x02);
    /*
    *   set ES7210 master reset control bits
    */
    for(cnt = 0; cnt < ADC_DEV_MAXNUM; cnt++)
    {
        if(es7210_private.tdm_mode[cnt] & 0x80)   //slave mode
        {
            es7210_update_bits(ES7210_RESET_CTL_REG00, 0x40, 0x40, es7210_private.i2c_chip_id[cnt]);
            es7210_update_bits(ES7210_CLK_ON_OFF_REG01, 0x61, 0x20, es7210_private.i2c_chip_id[cnt]);
        }
        else
        {//master mode
            es7210_update_bits(ES7210_RESET_CTL_REG00, 0x40, 0x00, es7210_private.i2c_chip_id[cnt]);
            es7210_update_bits(ES7210_CLK_ON_OFF_REG01, 0x61, 0x40, es7210_private.i2c_chip_id[cnt]);
        }
    }
#endif


    es7210_multi_chips_update_bits(ES7210_MODE_CFG_REG08, 0x04, 0x04);
}


void es7210_suspend(void)
{

}

void es7210_resume(void)
{

}

static int __attribute__((unused)) I2cInit(i2c_config_t *conf, int i2cMasterPort)
{
    int res;

    res = i2c_param_config(i2cMasterPort, conf);
    res |= i2c_driver_install(i2cMasterPort, conf->mode, 0, 0, 0);
    ES_ASSERT(res, "I2cInit error", -1);
    return res;
}

/*
* es7210_startup(), power up initialization
*
*   notes£º
* in this routine, we should set i2c chip address and work mode for every es7210 device in tdm link.
*   if the cpu i2s port is in slave mode, we set the es7210 device0 in i2s master mode as an i2s generator, others in slave mode.
*
* return 0, success
*
*/
int Es7210Init(void)
{
    unsigned int cnt;
    // I2cInit(&cfg->i2c_cfg, cfg->i2c_port_num);
    
    for(cnt =0; cnt < ADC_DEV_MAXNUM; cnt++)
    {
        es7210_private.i2c_chip_id[cnt] = 0x80 + cnt; // set i2c chip address
            es7210_private.tdm_mode[cnt] = ES7210_WORK_MODE_SLAVE;
    }

    es7210_init_codec();
    //for test

    // es7210_write(ES7210_MCLK_CTL_REG02, 0xC3, 0x41);
    // es7210_multi_chips_write(ES7210_MCLK_CTL_REG02, 0xc3);

    // es7210_multi_chips_write(0x02, 0xC2);
    // es7210_multi_chips_write(0x08, 0x10);
    // es7210_multi_chips_write(0x12, 0x02);
    // es7210_multi_chips_write(0x11, 0x60);
    return 0;
}
