/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_types.h"
#include "esp_err.h"

#include "driver/i2c.h"
#include "at581x_reg.h"


/* AT581X address */
#define AT581X_ADDRRES_0 (0x28<<1)

/**
 * @brief Type of AT581X device handle
 *
 */
typedef void *at581x_dev_handle_t;

/**
 * @brief AT581X I2C config struct
 *
 */
typedef struct {
    i2c_port_t  i2c_port;           /*!< I2C port used to connecte AT581X device */
    uint8_t     i2c_addr;           /*!< I2C address of AT581X device */
} at581x_i2c_config_t;

/**
 * @brief Init AT581X.
 *
 * @param[in]  handle AT581X device handle
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 */
esp_err_t at581x_init_sensor(at581x_dev_handle_t *handle);

/**
 * @brief Create new AT581X device handle.
 *
 * @param[in]  i2c_conf Config for I2C used by AT581X
 * @param[out] handle_out New AT581X device handle
 * @return
 *          - ESP_OK                  Device handle creation success.
 *          - ESP_ERR_INVALID_ARG     Invalid device handle or argument.
 *          - ESP_ERR_NO_MEM          Memory allocation failed.
 */
esp_err_t at581x_new_sensor(const at581x_i2c_config_t *i2c_conf, at581x_dev_handle_t *handle_out);

/**
 * @brief Delete AT581X device handle.
 *
 * @param handle AT581X device handle
 * @return
 *          - ESP_OK                  Device handle deletion success.
 *          - ESP_ERR_INVALID_ARG     Invalid device handle or argument.
 */
esp_err_t at581x_del_sensor(at581x_dev_handle_t handle);

/**
 * @brief Software reset.
 *
 * @param handle AT581X device handle
 * @return
 *          - ESP_OK                  Device handle deletion success.
 *          - ESP_ERR_INVALID_ARG     Invalid device handle or argument.
 */
esp_err_t at581x_soft_reset(at581x_dev_handle_t *handle);

/**
 * @brief Set the RF transmit frequency point.
 *
 * @param handle AT581X device handle
 * @param[in] freq_0x5f Refer to the frequency configuration table in at581x_reg.h
 * @param[in] freq_0x60 Refer to the frequency configuration table in at581x_reg.h
 * @return
 *          - ESP_OK                  Device handle deletion success.
 *          - ESP_ERR_INVALID_ARG     Invalid device handle or argument.
 */
esp_err_t at581x_set_freq_point(at581x_dev_handle_t *handle, uint8_t freq_0x5f, uint8_t freq_0x60);

/**
 * @brief Set self_test time after poweron.
 *
 * @note at581x_soft_reset is required after modification.
 *
 * @param handle AT581X device handle
 * @param[in] self_check_time time of self_test, unit: ms
 * @return
 *          - ESP_OK                  Device handle deletion success.
 *          - ESP_ERR_INVALID_ARG     Invalid device handle or argument.
 */
esp_err_t at581x_set_self_check_time(at581x_dev_handle_t *handle, uint32_t self_check_time);

/**
 * @brief Set the trigger base time (minimum 500ms).
 *
 * @param handle AT581X device handle
 * @param[in] base_time trigger base time, unit: ms
 * @return
 *          - ESP_OK                  Device handle deletion success.
 *          - ESP_ERR_INVALID_ARG     Invalid device handle or argument.
 */
esp_err_t at581x_set_trigger_base_time(at581x_dev_handle_t *handle, uint64_t base_time);

/**
 * @brief Set the protection time after triggering.
 *
 * @note There is no sensing function within the protection time.
 *
 * @param handle AT581X device handle
 * @param[in] protect_time protection time, unit: ms
 * @return
 *          - ESP_OK                  Device handle deletion success.
 *          - ESP_ERR_INVALID_ARG     Invalid device handle or argument.
 */
esp_err_t at581x_set_protect_time(at581x_dev_handle_t *handle, uint32_t protect_time);

/**
 * @brief Set detection distance.
 *
 * @param handle AT581X device handle
 * @param[in] pwr_40uA_switch The default current is about 68uA, minimum 40uA
 * @param[in] delta Threshold range: 0 ~ 1023
 * @param[in] gain Gain of radar, please adjust according to your radar's sensitivity
 * @return
 *          - ESP_OK                  Device handle deletion success.
 *          - ESP_ERR_INVALID_ARG     Invalid device handle or argument.
 */
esp_err_t at581x_set_distance(at581x_dev_handle_t *handle, bool pwr_40uA_switch, uint32_t delta, at581x_gain_t gain);

/**
 * @brief Set the delay time after the trigger.
 *
 * @param handle AT581X device handle
 * @param[in] keep_time delay time, unit: ms
 * @return
 *          - ESP_OK                  Device handle deletion success.
 *          - ESP_ERR_INVALID_ARG     Invalid device handle or argument.
 */
esp_err_t at581x_set_trigger_keep_time(at581x_dev_handle_t *handle, uint64_t keep_time);

/**
 * @brief Set light sensor (no light sensor by default).
 *
 * @param handle AT581X device handle
 * @param[in] onoff on/off the light sensor
 * @param[in] light_sensor_value_high threshold high
 * @param[in] light_sensor_value_low threshold low
 * @param[in] light_sensor_iniverse inverse
 *
 * @return
 *          - ESP_OK                  Device handle deletion success.
 *          - ESP_ERR_INVALID_ARG     Invalid device handle or argument.
 */
esp_err_t at581x_set_light_sensor_threshold(at581x_dev_handle_t *handle,
                                     bool onoff,
                                     uint32_t light_sensor_value_high,
                                     uint32_t light_sensor_value_low,
                                     uint32_t light_sensor_iniverse);
/**
 * @brief Control the RF module.
 *
 * @note After the RF module is off, it can save about 10uA power consumption.
 *
 * @param handle AT581X device handle
 * @param[in] onoff on/off the RF module
 * @return
 *          - ESP_OK                  Device handle deletion success.
 *          - ESP_ERR_INVALID_ARG     Invalid device handle or argument.
 */
esp_err_t at581x_set_rf_onoff(at581x_dev_handle_t *handle, bool onoff);

/**
 * @brief Set detect window length & threshold
 *
 * @param handle AT581X device handle
 * @param[in] window_length Number of windows per detection (Default: 4)
 * @param[in] window_threshold Number of windows needed to trigger (Default: 3)
 * @return
 *          - ESP_OK                  Device handle deletion success.
 *          - ESP_ERR_INVALID_ARG     Invalid device handle or argument.
 */
esp_err_t set_detect_window(at581x_dev_handle_t *handle, uint8_t window_length, uint8_t window_threshold);

#ifdef __cplusplus
}
#endif
