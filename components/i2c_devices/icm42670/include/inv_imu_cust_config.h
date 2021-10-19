/*
 * ________________________________________________________________________________________________________
 * Copyright (c) 2015-2015 InvenSense Inc. All rights reserved.
 *
 * This software, related documentation and any modifications thereto (collectively Software is subject
 * to InvenSense and its licensors' intellectual property rights under U.S. and international copyright
 * and other intellectual property rights laws.
 *
 * InvenSense and its licensors retain all intellectual property and proprietary rights in and to the Software
 * and any use, reproduction, disclosure or distribution of the Software without an express license agreement
 * from InvenSense is strictly prohibited.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE SOFTWARE IS
 * PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * INVENSENSE BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 * ________________________________________________________________________________________________________
 */
#ifndef _INV_ICM4N607_CONFIG_H_
#define _INV_ICM4N607_CONFIG_H_

#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>



#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define FIFO_WM_MODE_EN                      0 //1:fifo mode 0:dri mode
#define IS_HIGH_RES_MODE 					 0 // 1: FIFO high resolution mode (20bits data format); 0: 16 bits data format;
#define SPI_MODE_EN                          1 //1:spi 0:i2c

#define DATA_FORMAT_DPS_G                    1  // output sensor data with unit in default  accel: m/s2  gyro: rad/s  

#define SUPPORT_FIFO_TS                      1  // support fifo timestamp by default
#define DMP_POWERSAVE_FF                     0  // disable apex power save mode for apex freefall for better performance
#define DMP_POWERSAVE_PEDO                   1  // setting apex power save mode for apex pedometer  by default

#define SUPPORT_DELAY_US                     1  // 1: platform supports delay in us; 0: platform does not support delay in ms.

#define SENSOR_REG_DUMP                      0 //  register dump control
#define SENSOR_LOG_LEVEL                     3 //INV_LOG_LEVEL_INFO
#define SENSOR_LOG_TS_ONLY                   1 //only print log sensor data TS  , FIFO Timestampe is only available in fifo mode


#define SENSOR_DIRECTION                     0  // 0~7 sensorConvert map index below
/* { { 1, 1, 1},    {0, 1, 2} },
   { { -1, 1, 1},   {1, 0, 2} },
   { { -1, -1, 1},  {0, 1, 2} },
   { { 1, -1, 1},   {1, 0, 2} },
   { { -1, 1, -1},  {0, 1, 2} },
   { { 1, 1, -1},   {1, 0, 2} },
   { { 1, -1, -1},  {0, 1, 2} },
   { { -1, -1, -1}, {1, 0, 2} }, */

// For Xian the max FIFO size is 2048 Bytes; common FIFO package is 16 Bytes; so the max number of FIFO pakage is ~128.
#define MAX_RECV_PACKET                      128


/* Initial WOM threshold to be applied to IMU in mg */
#define WOM_THRESHOLD_INITIAL_MG 200

/* WOM threshold to be applied to IMU, ranges from 1 to 255, in 4mg unit */
#define WOM_THRESHOLD  WOM_THRESHOLD_INITIAL_MG/3
#define WOM_THRESHOLD_X  WOM_THRESHOLD
#define WOM_THRESHOLD_Y  WOM_THRESHOLD
#define WOM_THRESHOLD_Z  WOM_THRESHOLD


#if 0
/* customer board, need invoke system marco*/
#define INV_LOG           //define INV_LOG(loglevel,fmt,arg...)   printf("ICM4n607: "fmt"\n",##arg)  
//#define INV_LOG(loglevel,fmt, args...)   pr_err("INV icm4n607 "fmt"\n",##args)

//#define INV_MSG(loglevel,fmt, args...)   pr_err("INV icm4n607 "fmt"\n",##args) 
#define EXIT(a)  ;
#else
/* smart motion board*/
#include "Message.h"
#define INV_LOG           INV_MSG
#define EXIT(a)  exit(a)
#endif

/** @brief enumeration  of serial interfaces available on IMU */
typedef enum
{
	UI_I2C,
	UI_SPI4,
	UI_SPI3
} SERIAL_IF_TYPE_t;

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
