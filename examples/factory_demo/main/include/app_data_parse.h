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

#include <stdbool.h>
#include "app_led.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Get the cmd string object
 * 
 * @param id Command ID
 * @return String of given state
 */
char *get_cmd_string(int id);

/**
 * @brief Get the default led config object
 * 
 * @return Pointer to `led_state_t` 
 */
led_state_t *get_default_led_config(void);

/**
 * @brief Parse JSON string
 * 
 * @param text JSON string
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t app_wifi_parse_json_string(char *text);

/**
 * @brief Build JSON string with given LED state
 * 
 * @param led_state Status of LED
 * @param json_string Pointer to output buffer's pointer. Should free this buffer after used. 
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t app_wifi_build_json_string(led_state_t *led_state, char **json_string);

#ifdef __cplusplus
}
#endif
