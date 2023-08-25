/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @brief chip information definition
 */
#define CHIP_NAME                 "MoreSense AT581X"    /**< chip name */

/**
 * @brief Select gain for AT581X
 *
 * Range: 0x0000 ~ 0x1100 (the larger the value, the smaller the gain)
 * Step: 3db
 * Recommended: GAIN_3 ~ GAIN_9
 */
typedef enum {
    AT581X_STAGE_GAIN_0  = 0,
    AT581X_STAGE_GAIN_1,
    AT581X_STAGE_GAIN_2,
    AT581X_STAGE_GAIN_3,
    AT581X_STAGE_GAIN_4,
    AT581X_STAGE_GAIN_5,
    AT581X_STAGE_GAIN_6,
    AT581X_STAGE_GAIN_7,
    AT581X_STAGE_GAIN_8,
    AT581X_STAGE_GAIN_9,
    AT581X_STAGE_GAIN_A,
    AT581X_STAGE_GAIN_B,
    AT581X_STAGE_GAIN_C,
} at581x_gain_t;

/**
 * @brief Frequency Configuration Table
 */
#define FREQ_0X5F_5696MHZ           0x40
#define FREQ_0X60_5696MHZ           0x9d

#define FREQ_0X5F_5715MHZ           0x41
#define FREQ_0X60_5715MHZ           0x9d

#define FREQ_0X5F_5730MHZ           0x42
#define FREQ_0X60_5730MHZ           0x9d

#define FREQ_0X5F_5748MHZ           0x43
#define FREQ_0X60_5748MHZ           0x9d

#define FREQ_0X5F_5765MHZ           0x44
#define FREQ_0X60_5765MHZ           0x9d

#define FREQ_0X5F_5784MHZ           0x45
#define FREQ_0X60_5784MHZ           0x9d

#define FREQ_0X5F_5800MHZ           0x46
#define FREQ_0X60_5800MHZ           0x9d

#define FREQ_0X5F_5819MHZ           0x47
#define FREQ_0X60_5819MHZ           0x9d

#define FREQ_0X5F_5836MHZ           0x40
#define FREQ_0X60_5836MHZ           0x9e

#define FREQ_0X5F_5851MHZ           0x41
#define FREQ_0X60_5851MHZ           0x9e

#define FREQ_0X5F_5869MHZ           0x42
#define FREQ_0X60_5869MHZ           0x9e

#define FREQ_0X5F_5888MHZ           0x43
#define FREQ_0X60_5888MHZ           0x9e
