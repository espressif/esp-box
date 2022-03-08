/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
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

