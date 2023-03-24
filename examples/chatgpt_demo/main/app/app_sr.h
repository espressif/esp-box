/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#pragma once

#include <stdbool.h>
#include <sys/queue.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_models.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NEED_DELETE          BIT0
#define FEED_DELETED         BIT1
#define DETECT_DELETED       BIT2
#define HANDLE_DELETED       BIT3

#define SR_CONTINUE_DET 1
#define SR_RUN_TEST 0 /**< Just for sr experiment in laboratory >*/
#if SR_RUN_TEST
#ifdef SR_CONTINUE_DET
#undef SR_CONTINUE_DET
#define SR_CONTINUE_DET 0
#endif
#endif

typedef struct {
    wakenet_state_t wakenet_mode;
    esp_mn_state_t state;
    int command_id;
} sr_result_t;

typedef enum {
    SR_LANG_EN,
    SR_LANG_CN,
    SR_LANG_MAX,
} sr_language_t;


typedef struct {
    sr_language_t lang;
    model_iface_data_t *model_data;
    const esp_mn_iface_t *multinet;
    const esp_afe_sr_iface_t *afe_handle;
    esp_afe_sr_data_t *afe_data;
    int16_t *afe_in_buffer;
    int16_t *afe_out_buffer;
    uint8_t cmd_num;
    TaskHandle_t feed_task;
    TaskHandle_t detect_task;
    TaskHandle_t handle_task;
    QueueHandle_t result_que;
    EventGroupHandle_t event_group;
    FILE *fp;
    bool b_record_en;
} sr_data_t;

esp_err_t app_sr_start(bool record_en);
esp_err_t app_sr_stop(void);
esp_err_t app_sr_get_result(sr_result_t *result, TickType_t xTicksToWait);
esp_err_t app_sr_start_once(void);

#ifdef __cplusplus
}
#endif
