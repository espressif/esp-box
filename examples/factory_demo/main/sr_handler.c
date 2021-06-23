/**
 * @file sr_handler.c
 * @brief 
 * @version 0.1
 * @date 2021-08-31
 * 
 * @copyright Copyright 2021 Espressif Systems (Shanghai) Co. Ltd.
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "app_sr.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "lvgl.h"
#include "ui_main.h"

static const char *TAG = "sr_handler";

typedef enum {
    SR_CMD_REBOOT = 0,  /*!< ID - 00 */ /*!< Reboot */
    SR_CMD_CRASH,       /*!< ID - 01 */ /*!< Crash */
    SR_CMD_MEM_INFO,    /*!< ID - 02 */ /*!< Memory Information */
    SR_CMD_MAX,         /*!< Command list ends */
} sr_cmd_t;

void sr_task_handler(lv_timer_t *timer)
{
    LV_IMG_DECLARE(xiao_ai)
    static lv_obj_t *img = NULL;

    /* Create Alexa image object once */
    if (NULL == img) {
        img = lv_img_create(lv_scr_act());
        lv_img_set_src(img, &xiao_ai);
        lv_obj_align(img, LV_ALIGN_CENTER, 0, 80);
        lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
    }

    /* Show Alexa logo if wake word detected */
    if (app_sr_get_event_flag(SR_EVENT_WAKE)) {
        if (lv_obj_has_flag(img, LV_OBJ_FLAG_HIDDEN)) {
            lv_obj_clear_flag(img, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(img);
        }
    }

    /* Process wake word */
    if (app_sr_get_event_flag(SR_EVENT_WORD_DETECT)) {
        int32_t command_id = app_sr_get_command_id();

        /* Match command ID. See command list @sr_cmd_t. */
        switch (command_id) {
        case SR_CMD_REBOOT:     /* Rebbot */
            esp_restart();
            break;
        case SR_CMD_CRASH:      /* Crash */
            abort();
            break;
        case SR_CMD_MEM_INFO:   /* Memory Information */
            ESP_LOGI(TAG, "Free mem: %zu. Biggest block : %zu",
                heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
            break;
        default:
            ESP_LOGW(TAG, "Unsupported command : %d", command_id);
            break;
        }

        /* Clear detect flags */
        app_sr_clear_event_flag(SR_EVENT_WAKE);
        app_sr_clear_event_flag(SR_EVENT_WORD_DETECT);

        /* Hide Alexa logo */
        lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
    }
}
