/**
 * @file app_sr.h
 * @brief 
 * @version 0.1
 * @date 2021-09-19
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SR_EVENT_WAKE_UP = 1 << 0,
    SR_EVENT_WORD_DETECT = 1 << 1,
    SR_EVENT_TIMEOUT = 1 << 2,
    SR_EVENT_ALL =
        SR_EVENT_WAKE_UP | SR_EVENT_WORD_DETECT | SR_EVENT_TIMEOUT,
} sr_event_t;

/**
 * @brief 
 * 
 * @return esp_err_t 
 */
esp_err_t app_sr_start(void);

/**
 * @brief 
 * 
 * @return int32_t 
 */
int32_t app_sr_get_last_cmd_id(void);

/**
 * @brief 
 * 
 * @param pvParam 
 */
void sr_handler_task(void *pvParam);

#ifdef __cplusplus
}
#endif
