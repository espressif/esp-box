/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#pragma once

#include "bsp/esp-box-3.h"
#include "ui/ui.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "iot_button.h"
#include "app_ui_event.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BUTTON_TAG "BUTTON"

#define HC165D_PL    38
#define HC165D_CE    39
#define HC165D_SCL   40
#define HC165D_DATA  41

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (21) // Define the output GPIO
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY               (4095) // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#define LEDC_FREQUENCY          (5000) // Frequency in Hertz. Set frequency at 5 kHz

#define UP_BUTTON_MASK              (1UL << 12)
#define DOWN_BUTTON_MASK            (1UL << 13)
#define LEFT_BUTTON_MASK            (1UL << 14)
#define RIGHT_BUTTON_MASK           (1UL << 15)
#define LB_BUTTON_MASK              (1UL << 4)
#define LT_BUTTON_MASK              (1UL << 6)
#define SELECT_BUTTON_MASK          (1UL << 8)
#define A_BUTTON_MASK               (1UL << 0)
#define B_BUTTON_MASK               (1UL << 1)
#define X_BUTTON_MASK               (1UL << 2)
#define Y_BUTTON_MASK               (1UL << 3)
#define START_BUTTON_MASK           (1UL << 9)
#define RB_BUTTON_MASK              (1UL << 5)
#define RT_BUTTON_MASK              (1UL << 7)
#define LEFT_ROCKER_BUTTON_MASK     (1UL << 10)
#define RIGHT_ROCKER_BUTTON_MASK    (1UL << 11)

typedef struct {
    int channel_1_status;
    int channel_2_status;
    int channel_3_status;
    int channel_4_status;
} rc_channel_state_t;

void vibration_motor_init(void);
void box_rc_button_init(void);
void box_rc_button_delete(void);
uint32_t get_pressed_button_value(void);
rc_channel_state_t get_rc_button_state(void);

#ifdef __cplusplus
}
#endif
