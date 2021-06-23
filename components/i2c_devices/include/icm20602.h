/**
 * @file icm20602.h
 * @brief 
 * @version 0.1
 * @date 2021-01-14
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"

#include "bsp_i2c.h"
#include "i2c_bus.h"

#include "icm20602_reg.h"

#define ICM20602_ADDR   (0b1101000)

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	icm20602_gyro_range_250dps = 0,
	icm20602_gyro_range_500dps,
	icm20602_gyro_range_1000dps,
	icm20602_gyro_range_2000dps,
	icm20602_gyro_range_num,
} icm20602_gyro_range_t;

typedef enum {
    icm20602_acc_range_2g = 0,
    icm20602_acc_range_4g,
    icm20602_acc_range_8g,
    icm20602_acc_range_1g,
    icm20602_acc_range_num,
} icm20602_acc_range_t;

typedef struct {
    uint8_t reserve         : 1;
    uint8_t fifo_mode       : 1;
    uint8_t ext_sync_set    : 3;
    uint8_t dlpf_cfg        : 1;
} icm20602_config_reg_t;

typedef enum {
    ic20602_clk_internal_20m = 0,
    ic20602_clk_auto_1,
    ic20602_clk_auto_2,
    ic20602_clk_auto_3,
    ic20602_clk_auto_4,
    ic20602_clk_auto_5,
    ic20602_clk_internal_20m_6,
    ic20602_clk_stop,
} icm20602_clk_t;

typedef /** @brief Allowed value for accel DLPF bandwidth (ACCEL_CONFIG2 (0x1D) register) */
enum mpu_accel_bw {		// In the ACCEL_CONFIG2 (0x1D) register, the BW setting bits are :
	icm20602_acc_bw_218 = 1,	///< 001 = 218 Hz
	icm20602_acc_bw_99,			///< 010 = 99 Hz
	icm20602_acc_bw_45,			///< 011 = 45 Hz
	icm20602_acc_bw_21,			///< 100 = 21 Hz
	icm20602_acc_bw_10,			///< 101 = 10 Hz
	icm20602_acc_bw_5,			///< 110 = 5 Hz
	icm20602_acc_bw_420,		///< 111 = 420 Hz
	icm20602_acc_bw_num,
} icm20602_acc_bw_t;

/** @brief Allowed value for gyro DLPF bandwidth (CONFIG (0x1A) register) */
typedef enum {   // In the CONFIG register, the  BW setting bits are :
	icm20602_gyro_bw_176 = 1,   ///< 001 = 176 Hz
	icm20602_gyro_bw_92,        ///< 010 = 92 Hz
	icm20602_gyro_bw_41,        ///< 011 = 41 Hz
	icm20602_gyro_bw_20,        ///< 100 = 20 Hz
	icm20602_gyro_bw_10,        ///< 101 = 10 Hz
	icm20602_gyro_bw_5,         ///< 110 = 5 Hz
    icm20602_gyro_bw_num,
} icm20602_gyro_bw_t;

typedef struct {
    uint8_t device_reset    : 1;
    uint8_t device_sleep    : 1;
    uint8_t cycle_en        : 1;
    uint8_t standby_en      : 1;
    uint8_t temp_dis        : 1;
    uint8_t clk_sel         : 3;
} icm20602_power_reg_1_t;

typedef struct {
    uint8_t reserve : 2;
    uint8_t stby_xa : 1;
    uint8_t stby_ya : 1;
    uint8_t stby_za : 1;
    uint8_t stby_xg : 1;
    uint8_t stby_yg : 1;
    uint8_t stby_zg : 1;
} icm20602_power_reg_2_t;

typedef struct {
    //
} icm20602_config_t;

/**
 * @brief Initialize ICM-20602 with default config
 * 
 * @return esp_err_t 
 *      - ESP_OK : Initialize successfully
 */
esp_err_t icm20602_init(void);

#ifdef __cplusplus
}
#endif
