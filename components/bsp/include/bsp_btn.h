/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include "esp_err.h"
#include "button.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Call default button init code 
 * 
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_btn_init_default(void);

/**
 * @brief Register callback for button event
 * 
 * @param btn_id 0 based index of the button
 * @param event Event of button to register
 * @param callback Callback function
 * @param user_data User data
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_btn_register_callback(board_btn_id_t btn_id, button_event_t event, button_cb_t callback, void *user_data);

/**
 * @brief Remove all callbacks of the button
 * 
 * @param btn_id 0 based index of the button
 * @return 
 *    - ESP_OK: Success
 *    - ESP_ERR_INVALID_STATE: button not be initialize
 *    - ESP_ERR_INVALID_ARG: button id error
 */
esp_err_t bsp_btn_rm_all_callback(board_btn_id_t btn_id);

/**
 * @brief Get the press state of the button
 * 
 * @param btn_id 0 based index of the button
 * 
 * @return 
 *    - TRUE pressed state
 *    - FALSE released state
 */
bool bsp_btn_get_state(board_btn_id_t btn_id);

#ifdef __cplusplus
}
#endif
