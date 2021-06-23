/*
* ES8311.h  --  ES8311 ALSA SoC Audio Codec
*
* Authors:
*
* Based on ES8374.h by David Yang
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

#ifndef _ES7243_H
#define _ES7243_H
#include "ESCodec_common.h"

esp_err_t Es7243Init(void);
int Es7243ReadReg(uint8_t regAdd);

#endif
