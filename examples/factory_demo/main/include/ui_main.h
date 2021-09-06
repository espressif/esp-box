/**
 * @file ui_main.h
 * @brief 
 * @version 0.1
 * @date 2021-09-19
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 
 * 
 * @return esp_err_t 
 */
esp_err_t ui_main_start(void);

/**
 * @brief 
 * 
 * @param cmd_id 
 */
void ui_sr_show_cmd(int cmd_id);

/**
 * @brief 
 * 
 * @param text 
 */
void ui_sr_show_text(char *text);

/**
 * @brief 
 * 
 * @param show 
 */
void ui_sr_show_icon(bool show);

#ifdef __cplusplus
}
#endif
