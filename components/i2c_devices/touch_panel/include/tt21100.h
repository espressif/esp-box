/**
 * @file tt21100.h
 * @brief 
 * @version 0.1
 * @date 2021-09-06
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

#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Init TT21100 touch panel
 * 
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t tt21100_tp_init(void);

/**
 * @brief Read packet from TT21100
 * 
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t tt21100_tp_read(void);

/**
 * @brief 
 * 
 * @param p_tp_num 
 * @param p_x 
 * @param p_y 
 * @return esp_err_t 
 */
esp_err_t tt21100_get_touch_point(uint8_t *p_tp_num, uint16_t *p_x, uint16_t *p_y);

/**
 * @brief 
 * 
 * @param p_btn_val 
 * @param p_btn_signal 
 * @return esp_err_t 
 */
esp_err_t tt21100_get_btn_val(uint8_t *p_btn_val, uint16_t *p_btn_signal);

/**
 * @brief TT21100 will keep COMM_INT low until all data read.
 *        So if the INT line is low after read the packet, read again.
 * 
 * @return true Data avaliable
 * @return false All data has been read 
 */
bool tt21100_data_avaliable(void);

#ifdef __cplusplus
}
#endif

