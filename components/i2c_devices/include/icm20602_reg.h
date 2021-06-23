/**
 * @file icm20602_reg.h
 * @brief ICM20602 register map.
 * @version 0.1
 * @date 2021-03-07
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

#define MPUREG_XG_OFFS_USRH     (0x13)
#define MPUREG_YG_OFFS_USRH     (0x15)
#define MPUREG_ZG_OFFS_USRH     (0x17)
#define MPUREG_CONFIG           (0x1a)
#define MPUREG_SMPLRT_DIV       (0x19)
#define MPUREG_GYRO_CONFIG      (0x1b)
#define MPUREG_ACCEL_CONFIG     (0x1c)
#define MPUREG_ACCEL_CONFIG_2   (0x1d)
#define MPUREG_LP_CONFIG        (0x1e)
#define MPUREG_ACCEL_WOM_THR    (0x1f)
#define MPUREG_ACCEL_WOM_X_THR  (0x20)
#define MPUREG_ACCEL_WOM_Y_THR  (0x21)
#define MPUREG_ACCEL_WOM_Z_THR  (0x22)
#define MPUREG_FIFO_EN          (0x23)
#define MPUREG_INT_PIN_CFG      (0x37)
#define MPUREG_INT_ENABLE       (0x38)
#define MPUREG_INT_STATUS       (0x3a)
#define MPUREG_USER_CTRL        (0x6a)
#define MPUREG_PWR_MGMT_1       (0x6b)
#define MPUREG_PWR_MGMT_2       (0x6c)
#define MPUREG_BANK_SEL         (0x6d)
#define MPUREG_MEM_START_ADDR   (0x6e)
#define MPUREG_MEM_R_W          (0x6f)
#define MPUREG_FIFO_COUNTH      (0x72)
#define MPUREG_FIFO_R_W         (0x74)
#define MPUREG_WHO_AM_I         (0x75)
#define MPUREG_XA_OFFS_H        (0x77)
#define MPUREG_YA_OFFS_H        (0x7a)
#define MPUREG_ZA_OFFS_H        (0x7d)