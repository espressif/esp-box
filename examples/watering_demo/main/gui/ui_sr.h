/**
 * @file ui_main.c
 * @brief
 * @version 0.1
 * @date 2022-01-13
 *
 * @copyright Copyright 2022 Espressif Systems (Shanghai) Co. Ltd.
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

#pragma once

#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief
 *
 */
void ui_sr_anim_init(void);

/**
 * @brief
 *
 */
void sr_anim_start(void);

/**
 * @brief
 *
 */
void sr_anim_stop(void);

/**
 * @brief
 *
 * @param text
 */
void sr_anim_set_text(char *text);

#ifdef __cplusplus
}
#endif
