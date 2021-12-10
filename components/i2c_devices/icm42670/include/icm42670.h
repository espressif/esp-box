/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ICM_42670_H_
#define _ICM_42670_H_

#include "esp_err.h"
#include "inv_imu_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Init ICM-42670 IMU
 * 
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t icm42670_init(void);

/**
 * @brief Get raw data from ICM-42670
 * 
 * @param accData Pointer to accelerometer data
 * @param gyroData Pointor to gyroscope data
 * @param imu_chip_temperature Pointer to temperature data
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t icm42670_get_raw_data(AccDataPacket *accData,
                                GyroDataPacket *gyroData,
                                chip_temperature *imu_chip_temperature);


#ifdef __cplusplus
}
#endif

#endif

