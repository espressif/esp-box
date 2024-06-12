/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#pragma once

#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#ifdef __cplusplus
extern "C" {
#endif
#define ROCKER_TAG "ROCKER"

#define LEFT_HOTAS1_ADC_CHAN        ADC_CHANNEL_0
#define LEFT_HOTAS2_ADC_CHAN        ADC_CHANNEL_1
#define RIGHT_HOTAS1_ADC_CHAN       ADC_CHANNEL_2
#define RIGHT_HOTAS2_ADC_CHAN       ADC_CHANNEL_3

#define EXAMPLE_ADC_ATTEN           3
#define ADC_MEAS_WINDOW_SIZE        10

esp_err_t rocker_adc_init(void);
void get_rocker_adc_value_in_game_mode(uint16_t rocker_value[4]);
void get_rocker_adc_value_in_rc_mode(uint16_t rocker_value[4], uint8_t meas_count, float filter_coef);

#ifdef __cplusplus
}
#endif
