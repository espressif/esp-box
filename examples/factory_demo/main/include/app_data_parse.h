/**
 * @file app_data_parse.h
 * @brief 
 * @version 0.1
 * @date 2021-10-22
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

#include "cJSON.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 
 * 
 * @param text 
 * @return esp_err_t 
 */
esp_err_t app_wifi_parse_json_string(char *text);

/**
 * @brief 
 * 
 * @param led_state 
 * @param json_string 
 * @return esp_err_t 
 */
esp_err_t app_wifi_build_json_string(led_state_t *led_state, char **json_string);

#ifdef __cplusplus
}
#endif
