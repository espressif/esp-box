/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
// #include "bsp/esp-bsp.h"
#include "driver/i2s_std.h"
#include "iot_button.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BOARD_S3_BOX,
    BOARD_S3_BOX_LITE,
} boards_id_t;

typedef enum {
#if CONFIG_BSP_BOARD_ESP32_S3_BOX
    BOARD_BTN_ID_BOOT = 0,
    BOARD_BTN_ID_MUTE,
    BOARD_BTN_ID_HOME,
    BOARD_BTN_ID_NUM
#elif CONFIG_BSP_BOARD_ESP32_S3_BOX_Lite
    BOARD_BTN_ID_BOOT = 0,
    BOARD_BTN_ID_NUM
#endif
} bsp_button_id_t;

typedef struct {
    gpio_num_t row1[4]; //The first row
    gpio_num_t row2[4];
} pmod_pins_t;

typedef struct {
    /**
     * @brief ESP-Dev-Board SDMMC GPIO defination
     *
     * @note Only avaliable when PMOD connected
     */
    bool FUNC_SDMMC_EN   ;
    int SDMMC_BUS_WIDTH ;
    int GPIO_SDMMC_CLK  ;
    int GPIO_SDMMC_CMD  ;
    int GPIO_SDMMC_D0   ;
    int GPIO_SDMMC_D1   ;
    int GPIO_SDMMC_D2   ;
    int GPIO_SDMMC_D3   ;
    int GPIO_SDMMC_DET  ;

    /**
     * @brief ESP-Dev-Board SDSPI GPIO definationv
     *
     */
    bool FUNC_SDSPI_EN       ;
    int SDSPI_HOST          ;
    int GPIO_SDSPI_CS       ;
    int GPIO_SDSPI_SCLK     ;
    int GPIO_SDSPI_MISO     ;
    int GPIO_SDSPI_MOSI     ;

    /**
     * @brief ESP-Dev-Board SPI GPIO defination
     *
     */
    bool FUNC_SPI_EN         ;
    int GPIO_SPI_CS         ;
    int GPIO_SPI_MISO       ;
    int GPIO_SPI_MOSI       ;
    int GPIO_SPI_SCLK       ;

    /**
     * @brief ESP-Dev-Board RMT GPIO defination
     *
     */
    bool FUNC_RMT_EN         ;
    int GPIO_RMT_IR         ;
    int GPIO_RMT_LED        ;

    const pmod_pins_t *PMOD1;
    const pmod_pins_t *PMOD2;

} board_res_desc_t;

typedef esp_err_t (*bsp_codec_reconfig_fn)();

typedef esp_err_t (*bsp_codec_mute_fn)(bool enable);

typedef esp_err_t (*bsp_codec_volume_fn)(int volume, int *volume_set);

typedef esp_err_t (*bsp_i2s_reconfig_clk_fn)(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch);

typedef esp_err_t (*bsp_i2s_read_fn)(void *audio_buffer, size_t len, size_t *bytes_read, uint32_t timeout_ms);

typedef esp_err_t (*bsp_i2s_write_fn)(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms);

typedef struct {
    bsp_codec_mute_fn mute_set_fn;
    bsp_codec_volume_fn volume_set_fn;
    bsp_codec_reconfig_fn codec_reconfig_fn;

    bsp_i2s_read_fn i2s_read_fn;
    bsp_i2s_write_fn i2s_write_fn;
    bsp_i2s_reconfig_clk_fn i2s_reconfig_clk_fn;
} bsp_codec_config_t;

typedef struct {
    boards_id_t id;
    const char *name;
    /**
     * @brief Get board description
     */
    const board_res_desc_t *board_desc;

} boards_info_t;

/**
 * @brief Special config for dev board
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_board_init(void);

/**
 * @brief Get the board information
 *
 * @return pointer of boards_info_t
 */
const boards_info_t *bsp_board_get_info(void);

/**
 * @brief Get board description
 *
 * @return pointer of board_res_desc_t
 */
const board_res_desc_t *bsp_board_get_description(void);

/**
 * @brief Get the codec operation function
 *
 * @return pointer of bsp_codec_config
 */
bsp_codec_config_t *bsp_board_get_codec_handle(void);

/**
 * @brief Call default button init code
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t bsp_btn_init(void);

/**
 * @brief Register the button event callback function.
 *
 * @param btn A button handle to register
 * @param event Button event
 * @param callback Callback function.
 * @param user_data user data
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG   Arguments is invalid.
 */
esp_err_t bsp_btn_register_callback(bsp_button_id_t btn, button_event_t event, button_cb_t callback, void *user_data);

/**
 * @brief Unregister the button event callback function.
 *
 * @param btn A button handle to unregister
 *
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG   Arguments is invalid.
 */
esp_err_t bsp_btn_rm_all_callback(bsp_button_id_t btn);

#ifdef __cplusplus
}
#endif
