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
 
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "inv_imu_driver.h"
#define NUM_TODISCARD                 0
#define SELFTEST_SAMPLES_NUMBER       200
#define IMU_MAX_FIFO_SIZE        2000  // Max FIFO count size

#ifndef SUPPORT_PEDOMETER
#define SUPPORT_PEDOMETER 0
#endif

#ifndef SUPPORT_SELFTEST
#define SUPPORT_SELFTEST 0
#endif


#ifndef max
#define max(x, y)                     (x > y ? x : y)
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)                 (sizeof((a)) / sizeof((a)[0]))
#endif

#ifndef abs
#define abs(a)                        ((a) > 0 ? (a) : -(a)) /*!< Absolute value */
#endif

#define G                             		 9.80665
#define PI                            		 3.141592
#define TEMP_SENSITIVITY_1_BYTE		  		 0.4831f    // Temperature in Degrees Centigrade = (FIFO_TEMP_DATA / 2.07) + 25; for FIFO 1 byte temperature data conversion.
#define TEMP_SENSITIVITY_2_BYTE		  		 0.007548f  // Temperature in Degrees Centigrade = (TEMP_DATA / 132.48) + 25; for data register and FIFO high res. data conversion.
#define ROOM_TEMP_OFFSET              		 25         //Celsius degree

#define KSCALE_ACC_FSR           	  0.001197097f  // ACC_FSR * G / (65536.0f/2);
#define KSCALE_GYRO_FSR        		  0.001065264f  // GYR_FSR * PI / (180.0f * (65536.0f/2));

#define DEG2RAD(dps)				((dps) * PI / 180.0f)

#define SENSOR_DATA_BITS(HI_RES)    ((HI_RES) ? 20 : 16) /* internal use 19 bits, but it is 20 bits data */
#define ODR2SMPLRT_DIV(odr)			(1000/(odr) - 1)
//ACC:FSR_SEL=0->+/-2G, FSR_SEL=1->+/-4G, FSR_SEL=2->+/-8G, FSR_SEL=3->+/-16G
#define ACC_MIN_RANGE(FSR_SEL)		\
		(-(pow(2, (4 - (FSR_SEL)))) * G)
#define ACC_MAX_RANGE(FSR_SEL)		((pow(2, (4 - (FSR_SEL)))) * G)
#define ACC_RESOLUTION(FSR_SEL)		ACC_MAX_RANGE(FSR_SEL) / 32768.0f
#define ACC_RESOLUTION_HIRES(FSR_SEL,HI_RES)		(ACC_MAX_RANGE(FSR_SEL) / (float)(1 << (SENSOR_DATA_BITS(HI_RES) -1)))

//GYRO:FSR_SEL=0->+/-250dps, FSR_SEL=1->+/-500dps, FSR_SEL=2->+/-1000dps, FSR_SEL=3->+/-2000dps
#define GYRO_MIN_RANGE(FSR_SEL)		\
		(DEG2RAD(-15.625f * pow(2, (7 - (FSR_SEL)))))
#define GYRO_MAX_RANGE(FSR_SEL)		(DEG2RAD(15.625f * pow(2, (7 - (FSR_SEL)))))
#define GYRO_RESOLUTION(FSR_SEL)	GYRO_MAX_RANGE(FSR_SEL) / 32768.0f
#define GYRO_RESOLUTION_HIRES(FSR_SEL,HI_RES)	(GYRO_MAX_RANGE(FSR_SEL) / (float)(1 << (SENSOR_DATA_BITS(HI_RES) -1)))

/* wom config */
/** Default value for the WOM threshold
 *  Resolution of the threshold is 1g/256 ~= 3.9mg */

#define DEFAULT_WOM_THS_MG 52>>2 /* = 52mg/4 */
#define EN                 1
#define DIS                0

#if SUPPORT_PEDOMETER
/* pedo config */
/* LOW_ENERGY_AMP_TH_SEL */
typedef enum
{
    LOW_ENERGY_AMP_TH_SEL_1006632MG = (0 << BIT_LOW_ENERGY_AMP_TH_SEL_SHIFT),
    LOW_ENERGY_AMP_TH_SEL_1174405MG = (1 << BIT_LOW_ENERGY_AMP_TH_SEL_SHIFT),
    LOW_ENERGY_AMP_TH_SEL_1342177MG = (2 << BIT_LOW_ENERGY_AMP_TH_SEL_SHIFT),
    LOW_ENERGY_AMP_TH_SEL_1509949MG = (3 << BIT_LOW_ENERGY_AMP_TH_SEL_SHIFT),
    LOW_ENERGY_AMP_TH_SEL_1677721MG = (4 << BIT_LOW_ENERGY_AMP_TH_SEL_SHIFT),
    LOW_ENERGY_AMP_TH_SEL_1845493MG = (5 << BIT_LOW_ENERGY_AMP_TH_SEL_SHIFT),
    LOW_ENERGY_AMP_TH_SEL_2013265MG = (6 << BIT_LOW_ENERGY_AMP_TH_SEL_SHIFT),
    LOW_ENERGY_AMP_TH_SEL_2181038MG = (7 << BIT_LOW_ENERGY_AMP_TH_SEL_SHIFT),
    LOW_ENERGY_AMP_TH_SEL_2348810MG = (8 << BIT_LOW_ENERGY_AMP_TH_SEL_SHIFT),
    LOW_ENERGY_AMP_TH_SEL_2516582MG = (9 << BIT_LOW_ENERGY_AMP_TH_SEL_SHIFT),
    LOW_ENERGY_AMP_TH_SEL_2684354MG = (10 << BIT_LOW_ENERGY_AMP_TH_SEL_SHIFT),
    LOW_ENERGY_AMP_TH_SEL_2852126MG = (11 << BIT_LOW_ENERGY_AMP_TH_SEL_SHIFT),
    LOW_ENERGY_AMP_TH_SEL_3019898MG = (12 << BIT_LOW_ENERGY_AMP_TH_SEL_SHIFT),
    LOW_ENERGY_AMP_TH_SEL_3187671MG = (13 << BIT_LOW_ENERGY_AMP_TH_SEL_SHIFT),
    LOW_ENERGY_AMP_TH_SEL_3355443MG = (14 << BIT_LOW_ENERGY_AMP_TH_SEL_SHIFT),
    LOW_ENERGY_AMP_TH_SEL_3523215MG = (15 << BIT_LOW_ENERGY_AMP_TH_SEL_SHIFT)
} LOW_ENERGY_AMP_TH_t;

/* DMP_POWER_SAVE_TIME_SEL */
typedef enum
{
    DMP_POWER_SAVE_TIME_SEL_0S  = 0,
    DMP_POWER_SAVE_TIME_SEL_4S  = 1,
    DMP_POWER_SAVE_TIME_SEL_8S  = 2,
    DMP_POWER_SAVE_TIME_SEL_12S = 3,
    DMP_POWER_SAVE_TIME_SEL_16S = 4,
    DMP_POWER_SAVE_TIME_SEL_20S = 5,
    DMP_POWER_SAVE_TIME_SEL_24S = 6,
    DMP_POWER_SAVE_TIME_SEL_28S = 7,
    DMP_POWER_SAVE_TIME_SEL_32S = 8,
    DMP_POWER_SAVE_TIME_SEL_36S = 9,
    DMP_POWER_SAVE_TIME_SEL_40S = 10,
    DMP_POWER_SAVE_TIME_SEL_44S = 11,
    DMP_POWER_SAVE_TIME_SEL_48S = 12,
    DMP_POWER_SAVE_TIME_SEL_52S = 13,
    DMP_POWER_SAVE_TIME_SEL_56S = 14,
    DMP_POWER_SAVE_TIME_SEL_60S = 15
} DMP_POWER_SAVE_TIME_t;

/* PEDO_AMP_TH_SEL */
typedef enum
{
    PEDO_AMP_TH_1006632_MG = (0  << BIT_PED_AMP_TH_SEL_SHIFT),
    PEDO_AMP_TH_1140850_MG = (1  << BIT_PED_AMP_TH_SEL_SHIFT),
    PEDO_AMP_TH_1275068_MG = (2  << BIT_PED_AMP_TH_SEL_SHIFT),
    PEDO_AMP_TH_1409286_MG = (3  << BIT_PED_AMP_TH_SEL_SHIFT),
    PEDO_AMP_TH_1543503_MG = (4  << BIT_PED_AMP_TH_SEL_SHIFT),
    PEDO_AMP_TH_1677721_MG = (5  << BIT_PED_AMP_TH_SEL_SHIFT),
    PEDO_AMP_TH_1811939_MG = (6  << BIT_PED_AMP_TH_SEL_SHIFT),
    PEDO_AMP_TH_1946157_MG = (7  << BIT_PED_AMP_TH_SEL_SHIFT),
    PEDO_AMP_TH_2080374_MG = (8  << BIT_PED_AMP_TH_SEL_SHIFT),
    PEDO_AMP_TH_2214592_MG = (9  << BIT_PED_AMP_TH_SEL_SHIFT),
    PEDO_AMP_TH_2348810_MG = (10 << BIT_PED_AMP_TH_SEL_SHIFT),
    PEDO_AMP_TH_2483027_MG = (11 << BIT_PED_AMP_TH_SEL_SHIFT),
    PEDO_AMP_TH_2617245_MG = (12 << BIT_PED_AMP_TH_SEL_SHIFT),
    PEDO_AMP_TH_2751463_MG = (13 << BIT_PED_AMP_TH_SEL_SHIFT),
    PEDO_AMP_TH_2885681_MG = (14 << BIT_PED_AMP_TH_SEL_SHIFT),
    PEDO_AMP_TH_3019898_MG = (15 << BIT_PED_AMP_TH_SEL_SHIFT)
} PEDO_AMP_TH_t;

/* PED_STEP_CNT_TH_SEL */
typedef enum
{
    PED_STEP_CNT_TH_0_STEP = 0,
    PED_STEP_CNT_TH_1_STEP = 1,
    PED_STEP_CNT_TH_2_STEP = 2,
    PED_STEP_CNT_TH_3_STEP = 3,
    PED_STEP_CNT_TH_4_STEP = 4,
    PED_STEP_CNT_TH_5_STEP = 5,
    PED_STEP_CNT_TH_6_STEP = 6,
    PED_STEP_CNT_TH_7_STEP = 7,
    PED_STEP_CNT_TH_8_STEP = 8,
    PED_STEP_CNT_TH_9_STEP = 9,
    PED_STEP_CNT_TH_10_STEP = 10,
    PED_STEP_CNT_TH_11_STEP = 11,
    PED_STEP_CNT_TH_12_STEP = 12,
    PED_STEP_CNT_TH_13_STEP = 13,
    PED_STEP_CNT_TH_14_STEP = 14,
    PED_STEP_CNT_TH_15_STEP = 15,
} PED_STEP_CNT_TH_t;

/* PED_STEP_DET_TH_SEL */
typedef enum
{
    PED_STEP_DET_TH_0_STEP = (0 << BIT_PED_STEP_DET_TH_SEL_SHIFT),
    PED_STEP_DET_TH_1_STEP = (1 << BIT_PED_STEP_DET_TH_SEL_SHIFT),
    PED_STEP_DET_TH_2_STEP = (2 << BIT_PED_STEP_DET_TH_SEL_SHIFT),
    PED_STEP_DET_TH_3_STEP = (3 << BIT_PED_STEP_DET_TH_SEL_SHIFT),
    PED_STEP_DET_TH_4_STEP = (4 << BIT_PED_STEP_DET_TH_SEL_SHIFT),
    PED_STEP_DET_TH_5_STEP = (5 << BIT_PED_STEP_DET_TH_SEL_SHIFT),
    PED_STEP_DET_TH_6_STEP = (6 << BIT_PED_STEP_DET_TH_SEL_SHIFT),
    PED_STEP_DET_TH_7_STEP = (7 << BIT_PED_STEP_DET_TH_SEL_SHIFT),
} PED_STEP_DET_TH_t;

/* PEDO_SB_TIMER_TH_SEL */
typedef enum
{
    PEDO_SB_TIMER_TH_50_SAMPLES  = (0  << BIT_PED_SB_TIMER_TH_SEL_SHIFT),
    PEDO_SB_TIMER_TH_75_SAMPLES  = (1  << BIT_PED_SB_TIMER_TH_SEL_SHIFT),
    PEDO_SB_TIMER_TH_100_SAMPLES = (2  << BIT_PED_SB_TIMER_TH_SEL_SHIFT),
    PEDO_SB_TIMER_TH_125_SAMPLES = (3  << BIT_PED_SB_TIMER_TH_SEL_SHIFT),
    PEDO_SB_TIMER_TH_150_SAMPLES = (4  << BIT_PED_SB_TIMER_TH_SEL_SHIFT),
    PEDO_SB_TIMER_TH_175_SAMPLES = (5  << BIT_PED_SB_TIMER_TH_SEL_SHIFT),
    PEDO_SB_TIMER_TH_200_SAMPLES = (6  << BIT_PED_SB_TIMER_TH_SEL_SHIFT),
    PEDO_SB_TIMER_TH_225_SAMPLES = (7  << BIT_PED_SB_TIMER_TH_SEL_SHIFT)
} PEDO_SB_TIMER_TH_t;

/* PEDO_HI_ENRGY_TH_SEL */
typedef enum
{
    PEDO_HI_ENRGY_TH_90  = 0,
    PEDO_HI_ENRGY_TH_107 = 1,
    PEDO_HI_ENRGY_TH_136 = 2,
    PEDO_HI_ENRGY_TH_159 = 3
} PEDO_HI_ENRGY_TH_t;
#endif

/* Error List */
#define SENSOR_WHOAMI_INVALID_ERROR     -1
#define SENSOR_CONVERT_INVALID_ERROR    -2
#define SENSOR_INIT_INVALID_ERROR       -3
#define INT_STATUS_READ_ERROR           -4
#define DRDY_UNEXPECT_INT_ERROR         -5
#define DRDY_DATA_READ_ERROR            -6
#define DRDY_DATA_CONVERT_ERROR         -7
#define FIFO_COUNT_INVALID_ERROR        -8
#define FIFO_OVERFLOW_ERROR             -9
#define FIFO_UNEXPECT_WM_ERROR          -10
#define FIFO_DATA_READ_ERROR            -11
#define FIFO_DATA_CONVERT_ERROR         -12  
#define FIFO_DATA_FULL_ERROR            -13
#if SUPPORT_SELFTEST
#define SELFTEST_INIT_ERROR             -14
#define SELFTEST_ACC_ENABLE_ERROR       -15
#define SELFTEST_ACC_READ_ERROR         -16
#define SELFTEST_ACC_ST_READ_ERROR      -17
#define SELFTEST_ACC_STCODE_READ_ERROR  -18
#define SELFTEST_GYR_ENABLE_ERROR       -19
#define SELFTEST_GYR_READ_ERROR         -20
#define SELFTEST_GYR_ST_READ_ERROR      -21
#define SELFTEST_GYR_STCODE_READ_ERROR  -22
#endif
#define DMP_TIMEOUT_ERROR               -23
#define DMP_IDLE_ERROR                  -24
#define DMP_SIZE_ERROR                  -25
#define DMP_SRAM_ERROR                  -26
#define DMP_STATUS_ERROR                -27
#define DMP_LOAD_ERROR                  -28
#if SUPPORT_PEDOMETER
#define PEDO_ENABLE_ERROR               -29
#endif
#define WOM_ENABLE_ERROR                -32
#define FF_ENABLE_ERROR                 -33
#define FF_GETEVENT_ERROR               -34
#define PEDO_GETEVENT_ERROR             -35



#define AXIS_X          0
#define AXIS_Y          1
#define AXIS_Z          2
#define AXES_NUM        3

#define SENSOR_HZ(_hz)  ((uint32_t)((_hz) * 1024.0f))

#define ACCEL_DATA_SIZE               6
#define GYRO_DATA_SIZE                6
#define TEMP_DATA_SIZE                2

#define FIFO_HEADER_SIZE              1
#define FIFO_ACCEL_DATA_SHIFT         1
#define FIFO_ACCEL_DATA_SIZE          ACCEL_DATA_SIZE
#define FIFO_GYRO_DATA_SHIFT          7
#define FIFO_GYRO_DATA_SIZE           GYRO_DATA_SIZE
#define FIFO_TEMP_DATA_SHIFT          13
#define FIFO_TEMP_DATA_SIZE           1
#define FIFO_TIMESTAMP_DATA_SHIFT     (icm_dev.fifo_highres_enabled ? 15: 14)
#define FIFO_TIMESTAMP_SIZE           2
#define FIFO_TEMP_HIGH_RES_SIZE       1
#define FIFO_ACCEL_GYRO_HIGH_RES_SIZE 3
#define FIFO_ACCEL_GYRO_HIGH_RES_DATA_SHIFT  FIFO_HEADER_SIZE + FIFO_ACCEL_DATA_SIZE + FIFO_GYRO_DATA_SIZE + FIFO_TEMP_DATA_SIZE + FIFO_TEMP_HIGH_RES_SIZE +FIFO_TIMESTAMP_SIZE 

#define FIFO_EMPTY_PACKAGE_SIZE       0
#define FIFO_8BYTES_PACKET_SIZE       (FIFO_HEADER_SIZE + FIFO_ACCEL_DATA_SIZE + FIFO_TEMP_DATA_SIZE)
#define FIFO_16BYTES_PACKET_SIZE      (FIFO_HEADER_SIZE + FIFO_ACCEL_DATA_SIZE + FIFO_GYRO_DATA_SIZE + FIFO_TEMP_DATA_SIZE + FIFO_TIMESTAMP_SIZE)
#define FIFO_20BYTES_PACKET_SIZE      (FIFO_HEADER_SIZE + FIFO_ACCEL_DATA_SIZE + FIFO_GYRO_DATA_SIZE + FIFO_TEMP_DATA_SIZE + FIFO_TIMESTAMP_SIZE +\
                                       FIFO_TEMP_HIGH_RES_SIZE + FIFO_ACCEL_GYRO_HIGH_RES_SIZE)
#define FIFO_HEADER_EMPTY_BIT         0x80
#define FIFO_HEADER_A_BIT             0x40
#define FIFO_HEADER_G_BIT             0x20
#define FIFO_HEADER_20_BIT            0x10

#define DRI_ACCEL_DATA_SHIFT          2
#define DRI_GYRO_DATA_SHIFT           8
#define DRI_14BYTES_PACKET_SIZE       (TEMP_DATA_SIZE + ACCEL_DATA_SIZE + GYRO_DATA_SIZE)

typedef enum {
    ODR_8KHZ    = 3,
    ODR_4KHZ    = 4,
    ODR_2KHZ    = 5,
    ODR_1KHZ    = 6,
    ODR_200HZ   = 7,
    ODR_100HZ   = 8,
    ODR_50HZ    = 9,
    ODR_25HZ    = 10,
    ODR_12_5HZ  = 11,
    ODR_500HZ   = 15,
} imu_sensor_odr_t;
    

/** @brief IMU max FSR values for accel and gyro
 *  Dependent on chip
 */
#if defined(ICM43686)
	#define ACCEL_CONFIG0_FS_SEL_MAX ACCEL_CONFIG0_FS_SEL_32g
	#define GYRO_CONFIG0_FS_SEL_MAX  GYRO_CONFIG0_FS_SEL_4000dps

	#define ACCEL_OFFUSER_MAX_MG 2000 
	#define GYRO_OFFUSER_MAX_DPS 128 

#else
	#define ACCEL_CONFIG0_FS_SEL_MAX ACCEL_CONFIG0_FS_SEL_16g
	#define GYRO_CONFIG0_FS_SEL_MAX  GYRO_CONFIG0_FS_SEL_2000dps

	#define ACCEL_OFFUSER_MAX_MG 1000 
	#define GYRO_OFFUSER_MAX_DPS 64

#endif

/* Filter order */
typedef enum {
    FIRST_ORDER = 0,    // 1st order
    SEC_ORDER   = 1,    // 2nd order
    THIRD_ORDER = 2,    // 3rd order
} imu_filter_order_t;

#if 0
typedef enum {
    I2C_SLEW_RATE_20_60NS = 0 << BIT_I2C_SLEW_RATE_SHIFT,
    I2C_SLEW_RATE_12_36NS = 1 << BIT_I2C_SLEW_RATE_SHIFT,
    I2C_SLEW_RATE_6_18NS  = 2 << BIT_I2C_SLEW_RATE_SHIFT,
    I2C_SLEW_RATE_4_12NS  = 3 << BIT_I2C_SLEW_RATE_SHIFT,
    I2C_SLEW_RATE_2_6NS   = 4 << BIT_I2C_SLEW_RATE_SHIFT,
    I2C_SLEW_RATE_2NS     = 5 << BIT_I2C_SLEW_RATE_SHIFT,
    SPI_SLEW_RATE_20_60NS = 0,
    SPI_SLEW_RATE_12_36NS = 1,
    SPI_SLEW_RATE_6_18NS  = 2,
    SPI_SLEW_RATE_4_12NS  = 3,
    SPI_SLEW_RATE_2_6NS   = 4,
    SPI_SLEW_RATE_2NS     = 5,
} imu_slew_rate_t;
#endif

/* sensor bandwidth */
typedef enum {
    BW_ODR_DIV_2  = 0,
    BW_ODR_DIV_4  = 1,
    BW_ODR_DIV_5  = 2,
    BW_ODR_DIV_8  = 3,
    BW_ODR_DIV_10 = 4,
    BW_ODR_DIV_16 = 5,
    BW_ODR_DIV_20 = 6,
    BW_ODR_DIV_40 = 7,
} imu_bandwidth_t;

static uint8_t IMU_ODR_MAPPING[] = {
    ACCEL_CONFIG0_ODR_12_5_HZ,
    ACCEL_CONFIG0_ODR_25_HZ,
    ACCEL_CONFIG0_ODR_50_HZ,
    ACCEL_CONFIG0_ODR_100_HZ,
    ACCEL_CONFIG0_ODR_200_HZ,
    ACCEL_CONFIG0_ODR_400_HZ,
    ACCEL_CONFIG0_ODR_800_HZ,
    ACCEL_CONFIG0_ODR_1600_HZ,
};

/* Support odr range 25HZ - 800HZ */
static uint32_t IMUHWRates[] = {
    SENSOR_HZ(12.5f),
    SENSOR_HZ(25.0f),
    SENSOR_HZ(50.0f),
    SENSOR_HZ(100.0f),
    SENSOR_HZ(200.0f),
    SENSOR_HZ(400.0f),
    SENSOR_HZ(800.0f),
    SENSOR_HZ(1600.0f),
};

/* chip type */
typedef enum {
    INVALID_TYPE = 0,
    CHIP_ICM42607,
    CHIP_ICM42670,	
    CHIP_ICM43607,
    CHIP_T1000,
} chip_type_t;

struct sensorConvert {
    int8_t    sign[3];
    uint8_t   axis[3];
};

 struct sensorConvert inv_map[] = {
    { { 1, 1, 1},    {0, 1, 2} },
    { { -1, 1, 1},   {1, 0, 2} },
    { { -1, -1, 1},  {0, 1, 2} },
    { { 1, -1, 1},   {1, 0, 2} },

    { { -1, 1, -1},  {0, 1, 2} },
    { { 1, 1, -1},   {1, 0, 2} },
    { { 1, -1, -1},  {0, 1, 2} },
    { { -1, -1, -1}, {1, 0, 2} },
};

typedef struct {
    uint32_t rate; //the rate from up layer want
    uint32_t hwRate; // the rate currently set
    uint32_t preRealRate; //the rate from sensor list sync with upper layer want
    uint32_t samplesToDiscard; // depends on acc or gyro start up time to valid data
    uint16_t wm; //the watermark bytes number setting for watermark from user
    bool powered;
    bool configed;
    bool needDiscardSample;
} IMUSensor_t;

typedef struct inv_imu {
    IMUSensor_t sensors[NUM_OF_SENSOR-1];   //TS is not a real sensor
    int  (*read_reg)(void * context, uint8_t reg, uint8_t * buf, uint32_t len);
    int  (*write_reg)(void * context, uint8_t reg, const uint8_t * buf, uint32_t len);

    void (*delay_ms)(uint32_t);
#if SUPPORT_DELAY_US
    void (*delay_us)(uint32_t);
#endif
    
    chip_type_t product;
    uint8_t serif_type;
    bool fifo_is_used;                      /**< Data are get from FIFO or from sensor registers. By default Fifo is used*/
    float    chip_temper;
    bool     tmst_is_used;
    bool     dmp_is_on;
    
    /* For save reg status */
    uint8_t  int_cfg;
    int_source0_t  int_src0;
    pwr_mgmt0_t  pwr_mgmt;
    accel_config0_t  acc_cfg0;
    gyro_config0_t  gyro_cfg0;

    /* For fifo */
    uint16_t watermark;
    uint8_t  dataBuf[IMU_MAX_FIFO_SIZE];
    uint32_t fifoDataToRead;
    bool     fifo_highres_enabled;            /**< FIFO packets are 20 bytes long */
	bool     polling_data_en;
    uint16_t fifo_packet_size;
    uint16_t dri_packet_size;

    uint8_t need_mclk_cnt;
    uint16_t pre_fifo_ts;
    uint64_t totol_sensor_ts;
    uint32_t min_apex_odr;
    uint32_t pre_min_apex_odr;        // record previous min apex_odr
    uint32_t pre_hwRate;
    
    bool     init_cfg;
    /* For sensor oriention convert, sync to Andriod coordinate */
    struct sensorConvert cvt;
    
#if SUPPORT_SELFTEST  
    int      avg[AXES_NUM];
    int      acc_avg[AXES_NUM];
    int      acc_avg_st[AXES_NUM];
    int      gyro_avg[AXES_NUM];
    int      gyro_avg_st[AXES_NUM];

    uint8_t  gyro_config;
    uint8_t  accel_config;
    uint8_t  st_rawdata_reg;
    uint8_t  acc_st_code[AXES_NUM];
    uint8_t  gyro_st_code[AXES_NUM];
#endif

    bool     dmp_power_save;
    bool     apex_enable;

} inv_imu_t;

static inv_imu_t icm_dev;

/* ****************************funcntion pre def  **********************/

static int inv_imu_convert_rawdata(struct accGyroDataPacket *packet);

static int inv_imu_switch_on_mclk(void);
static int inv_imu_switch_off_mclk(void);
int inv_imu_read_reg(uint32_t reg, uint32_t len, uint8_t * buf);
int inv_imu_write_reg(uint32_t reg, uint32_t len, const uint8_t * buf);
int inv_imu_set_timestamp_resolution(const TMST_CONFIG1_RESOL_t timestamp_resol);
int inv_imu_enable_accel_low_power_mode(uint32_t accel_rate_us);
int inv_imu_enable_accel_low_noise_mode(uint32_t accel_rate_us);
int inv_imu_config_drdy(bool enable);
int inv_imu_config_fifo_int(bool enable);
int inv_imu_config_fifofull_int(bool enable);
void inv_apply_mounting_matrix(int32_t raw[3]);
int inv_imu_device_reset(void);
int inv_imu_configure_fifo_interface(void);
int inv_imu_configure_wom(const uint8_t wom_x_th, const uint8_t wom_y_th, const uint8_t wom_z_th,
                               WOM_CONFIG_WOM_INT_MODE_t wom_int, WOM_CONFIG_WOM_INT_DUR_t wom_dur);
int inv_imu_process_selftest_end(inv_imu_selftest_output_t *st_output);
int inv_imu_control_selftest(uint8_t acc_control,uint8_t gyro_control);
int inv_imu_configure_selftest_parameters(void);
int inv_imu_reset_dmp(void);
int inv_imu_start_dmp_selftest(void);
int inv_imu_reset_fifo(void);
int inv_imu_read_rawdata(void);
int inv_imu_polling_rawdata(struct accGyroDataPacket *dataPacket);
int inv_imu_disable_wom_register(bool wom_disable);
int inv_imu_enable_wom_register(void);
int inv_imu_apex_init_parameters_struct( inv_imu_apex_parameters_t *apex_inputs);
int inv_imu_apex_configure_parameters(const inv_imu_apex_parameters_t *apex_inputs);


/* ****************************funcntion realization  **********************/

void inv_imu_set_serif(int (*read)(void *, uint8_t, uint8_t *, uint32_t),
                            int (*write)(void *, uint8_t, const uint8_t *, uint32_t))
{
    if (read == NULL || write == NULL) {
        INV_LOG(INV_LOG_LEVEL_ERROR, "Read/Write API NULL POINTER!!");
        EXIT(-1);
    }
    icm_dev.read_reg = read;
    icm_dev.write_reg = write;
}

static int inv_read(uint8_t addr, uint32_t len, uint8_t *buf)
{   
    return icm_dev.read_reg(0, addr, buf, len);
}

static int inv_write(uint8_t addr, uint32_t len, const uint8_t *buf)
{
    return icm_dev.write_reg(0, addr, buf, len);
}

void inv_imu_set_delay(void (*delay_ms)(uint32_t), void (*delay_us)(uint32_t))
{
#if SUPPORT_DELAY_US
    if (delay_ms == NULL || delay_us == NULL) {
        INV_LOG(INV_LOG_LEVEL_ERROR, "DELAY API NULL POINTER!!");
        EXIT(-1);
    }
    icm_dev.delay_ms = delay_ms;
    icm_dev.delay_us = delay_us;
#else
 if (delay_ms == NULL ) {
        INV_LOG(INV_LOG_LEVEL_ERROR, "DELAY MS API NULL POINTER!!");
        EXIT(-1);
    }
    icm_dev.delay_ms = delay_ms;
 #endif
}

static void inv_delay_ms(uint16_t ms)
{
    icm_dev.delay_ms(ms);
}

static void inv_delay_us(uint16_t us)
{
#if SUPPORT_DELAY_US
    icm_dev.delay_us(us);
#else
	icm_dev.delay_ms(1);       // When there is no microsecond delay per platform, we delay minimal 1ms.
#endif
}

static int read_mclk_reg(uint16_t regaddr, uint8_t rd_cnt, uint8_t * buf)
{
	uint8_t data;
	uint8_t blk_sel = (regaddr&0xFF00)>>8;
	int status = 0;

	// Have IMU not in IDLE mode to access MCLK domain
	status |= inv_imu_switch_on_mclk();

    if(blk_sel == 0x10){
        blk_sel = 0x00;
	}
	// optimize by changing BLK_SEL only if not NULL
	if(blk_sel)
		status |= inv_write((uint8_t)BLK_SEL_R & 0xff, 1, &blk_sel);

	data = (regaddr&0x00FF);
	status |= inv_write((uint8_t)MADDR_R, 1, &data);
	// wait 16 MCLK (4MHz) clock cycles
	inv_delay_us(10);
	status |= inv_read((uint8_t)M_R, rd_cnt, buf);
	// wait 16 MCLK (4MHz) clock cycles
	inv_delay_us(10);
	
	if(blk_sel) {
		data = 0;
		status |= inv_write((uint8_t)BLK_SEL_R, 1, &data);
	}

	// switch OFF MCLK if needed
	status |= inv_imu_switch_off_mclk();

	return status;
}

static int write_mclk_reg(uint16_t regaddr, uint8_t wr_cnt, const uint8_t * buf)
{
	uint8_t data;
	uint8_t blk_sel = (regaddr&0xFF00)>>8;
	uint8_t i = 0;
	int status = 0;

	// Have IMU not in IDLE mode to access MCLK domain
	status |= inv_imu_switch_on_mclk();
    if(blk_sel == 0x10){
        blk_sel = 0x00;
	}
	// optimize by changing BLK_SEL only if not NULL
	if(blk_sel)
		status |= inv_write((uint8_t)BLK_SEL_W, 1, &blk_sel);
	
	data = (regaddr&0x00FF);
	status |= inv_write((uint8_t)MADDR_W, 1, &data);
	for (i = 0; i<wr_cnt; i++) {
		status |= inv_write((uint8_t)M_W, 1, &buf[i]);
		// wait 16 MCLK (4MHz) clock cycles
		inv_delay_us(10);
	}
	
	if(blk_sel) {
		data = 0;
		status = inv_write((uint8_t)BLK_SEL_W, 1, &data);
	}

	status |= inv_imu_switch_off_mclk();

	return status;
}

int inv_imu_read_reg(uint32_t reg, uint32_t len, uint8_t * buf)
{
	uint32_t i=0;
	int rc = 0;
	
	if(reg & 0xff00) {
	    for(i=0; i<len ; i++){
		    rc |= read_mclk_reg(((reg+i)&0xFFFF), 1, &buf[i]);
		}
	} else {
		rc |= inv_read((uint8_t)reg, len, buf);
		}

	return rc;
}

int inv_imu_write_reg(uint32_t reg, uint32_t len, const uint8_t * buf)
{
	uint32_t i;
	int rc = 0;

	if(reg & 0xff00) {
	    for(i=0; i<len; i++) {
		    rc |= write_mclk_reg(((reg+i)&0xFFFF), 1, &buf[i]);
		}
	}else{	
		rc |= inv_write((uint8_t)reg, len, buf);
        //INV_LOG(INV_LOG_LEVEL_ERROR, "direct write reg %x len %x, value %x",reg,len,buf[0]);
    }
	return rc;
}

/*
 * Static functions definition
 */

static int inv_imu_switch_on_mclk(void)
{
	int status = 0;
	pwr_mgmt0_t reg_pwr_mgmt0;
 	mclk_rdy_t mclk_rdy;

    //INV_LOG(INV_LOG_LEVEL_ERROR, "inv_imu_switch_on_mclk mclk_cnt %d !!",icm_dev.need_mclk_cnt );
    
	/* set IDLE bit only if it is not set yet */
	if (icm_dev.need_mclk_cnt == 0) {

		status |= inv_imu_read_reg(PWR_MGMT_0, 1, (uint8_t*)&reg_pwr_mgmt0);
		reg_pwr_mgmt0.idle = PWR_MGMT_0_IDLE_RCOSC_ON ; 
		status |= inv_imu_write_reg(PWR_MGMT_0, 1, (uint8_t*)&reg_pwr_mgmt0);
		// INV_LOG(INV_LOG_LEVEL_ERROR, "inv_imu_switch_on_mclk PWR_MGMT_0 %x !!",reg_pwr_mgmt0);
		if(status)
			return status;

		/* Check if MCLK is ready */
		do{
			status = inv_imu_read_reg(MCLK_RDY, 1, (uint8_t*)&mclk_rdy);
		} while ((status != 0) || (MCLK_RDY_NO ==mclk_rdy.mclk_rdy));
	} else {

		/* Make sure it is already on */
		status |= inv_imu_read_reg(PWR_MGMT_0, 1, (uint8_t*)&reg_pwr_mgmt0);
		if (PWR_MGMT_0_IDLE_RCOSC_OFF == reg_pwr_mgmt0.idle){
			status |= INV_ERROR;
            INV_LOG(INV_LOG_LEVEL_ERROR, "mismatch status in inv_imu_switch_on_mclk!! %d count %d",reg_pwr_mgmt0.idle,icm_dev.need_mclk_cnt);
		}
	}

	/* Increment the counter to keep track of number of MCLK requesters */
	icm_dev.need_mclk_cnt++;
	
	return status;	
}

static int inv_imu_switch_off_mclk(void)
{
	int status = 0;
    volatile pwr_mgmt0_t reg_pwr_mgmt0;

   // INV_LOG(INV_LOG_LEVEL_ERROR, "inv_imu_switch_off_mclk mclk_cnt %d !!",icm_dev.need_mclk_cnt );
    
	/* Reset the IDLE but only if there is one requester left */
	if (icm_dev.need_mclk_cnt == 1) {
		status |= inv_imu_read_reg(PWR_MGMT_0, 1, (uint8_t*)&reg_pwr_mgmt0);
		reg_pwr_mgmt0.idle = PWR_MGMT_0_IDLE_RCOSC_OFF;
		status |= inv_imu_write_reg(PWR_MGMT_0, 1, (uint8_t*)&reg_pwr_mgmt0);
        //INV_LOG(INV_LOG_LEVEL_ERROR, "inv_imu_switch_off_mclk == W PWR_MGMT_0 %x !!",reg_pwr_mgmt0);
	} else {
		/* Make sure it is still on */
		status |= inv_imu_read_reg(PWR_MGMT_0, 1, (uint8_t*)&reg_pwr_mgmt0);
		if (PWR_MGMT_0_IDLE_RCOSC_OFF == reg_pwr_mgmt0.idle){
			status |= INV_ERROR;
            INV_LOG(INV_LOG_LEVEL_ERROR, "mismatch status in inv_imu_switch_off_mclk!! %d count %d",reg_pwr_mgmt0.idle,icm_dev.need_mclk_cnt);
		}
	}

	/* Decrement the counter 
   */
	if(icm_dev.need_mclk_cnt != 0)
		icm_dev.need_mclk_cnt--;
	
	return status;
}

static void inv_imu_get_whoami(void)
{
    int j,ret = 0;
    uint8_t data;

    INV_LOG(SENSOR_LOG_LEVEL, "inv_imu_get_whoami");
    for (j = 0; j < 3; j++) {
        ret = inv_imu_read_reg(WHO_AM_I, 1, &data);
        INV_LOG(SENSOR_LOG_LEVEL, "Chip Id is 0x%x ret is %d", data, ret);
    }

    switch (data) {
    case ICM42607_WHOAMI:
        {
            icm_dev.product = CHIP_ICM42607;
            INV_LOG(SENSOR_LOG_LEVEL, "CHIP_ICM42607 detected");
        }
        break;
    case ICM43607_WHOAMI:
        {
            icm_dev.product = CHIP_ICM43607;
            INV_LOG(SENSOR_LOG_LEVEL, "CHIP_ICM43607 detected");
        }
        break;
    case T1000_WHOAMI:
        {
            icm_dev.product = CHIP_T1000;
            INV_LOG(SENSOR_LOG_LEVEL, "CHIP_T1000 detected");
        }
        break;
    case ICM42670_WHOAMI:
        {
            icm_dev.product = CHIP_ICM42670;
            INV_LOG(SENSOR_LOG_LEVEL, "ICM42670 detected");
        }
        break;
    default:
        {
            icm_dev.product = INVALID_TYPE;
            INV_LOG(SENSOR_LOG_LEVEL, "Chip not supported");
        }
        return;
    }
}

 /* Static functions definition
 */
static int configure_serial_interface(void)
{
	intf_config1_t reg_intf_config1;
    device_config_t device_config;
    intf_config0_t reg_intf_config0;
	int status = 0;

	switch(icm_dev.serif_type) {

	case UI_I2C:
		// Enable I2C
		status |= inv_imu_read_reg(INTF_CONFIG1, 1, (uint8_t*)&reg_intf_config1);
		reg_intf_config1.i3c_ddr_en = INTF_CONFIG1_I3C_DDR_DIS;
		reg_intf_config1.i3c_sdr_en = INTF_CONFIG1_I3C_SDR_DIS;
	
		status |= inv_imu_write_reg(INTF_CONFIG1, 1, (uint8_t*)&reg_intf_config1);

	    // disable SPI 
		status |= inv_imu_read_reg(INTF_CONFIG0, 1, (uint8_t*)&reg_intf_config0);
		reg_intf_config0.ui_sifs_cfg = INTF_CONFIG0_UI_SIFS_CFG_DISABLE_SPI;	
		status |= inv_imu_write_reg(INTF_CONFIG0, 1, (uint8_t*)&reg_intf_config0);
		break;
            
	case UI_SPI4:
		device_config.spi_mode = DEVICE_CONFIG_SPI_MODE_0_3;
		// Force chip in SPI4W, no read-modify-write as MISO can't be trusted
	    device_config.spi_ap_4wire = DEVICE_CONFIG_SPI_AP_4WIRE;
		status |= inv_imu_write_reg(DEVICE_CONFIG_REG, 1, (uint8_t*)&device_config);
				status |= inv_imu_read_reg(DEVICE_CONFIG_REG, 1, (uint8_t*)&device_config);
		// disable I2C 
		status |= inv_imu_read_reg(INTF_CONFIG0, 1, (uint8_t*)&reg_intf_config0);
		reg_intf_config0.ui_sifs_cfg= INTF_CONFIG0_UI_SIFS_CFG_DISABLE_I2C;	
		status |= inv_imu_write_reg(INTF_CONFIG0, 1, (uint8_t*)&reg_intf_config0);
		break;

	case UI_SPI3:

		break;

	default:
		status |= INV_ERROR_BAD_ARG;
	}

	return status;
}

static int select_rcosc(void)
{
	int status = 0;

	status |= inv_imu_read_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);
	icm_dev.pwr_mgmt.accel_lp_clk_sel = PWR_MGMT_0_ACCEL_LP_CLK_RCOSC;
    status |= inv_imu_write_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);
    INV_LOG(INV_LOG_LEVEL_ERROR, "select_rcosc PWR_MGMT_0 %x !!",icm_dev.pwr_mgmt);
	return status;
}

#if 0
static int select_wuosc(void)
{
	int status = 0;

	status |= inv_imu_read_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);
	icm_dev.pwr_mgmt.accel_lp_clk_sel = PWR_MGMT_0_ACCEL_LP_CLK_WUOSC;
	status |= inv_imu_write_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);
    INV_LOG(INV_LOG_LEVEL_ERROR, "select_wuosc PWR_MGMT_0 %x !!",icm_dev.pwr_mgmt);

	return status;

}
#endif
 

static int reload_otp(void)
{
	int status = INV_ERROR_SUCCESS;
	otp_config_t otp_config;
    otp_ctrl7_t otp_ctrl7;
    
	/*status |= inv_imu_switch_on_mclk(); */

	status |= inv_imu_read_reg(OTP_CONFIG, 1, (uint8_t*)&otp_config);
	otp_config.otp_copy_mode = OTP_COPY_EN_TRIM;
    status |= inv_imu_write_reg(OTP_CONFIG, 1, (uint8_t*)&otp_config);

    INV_LOG(INV_LOG_LEVEL_INFO, " otp w OTP_CONFIG %x !!",otp_config);

    status |= inv_imu_read_reg( OTP_CTRL7, 1, (uint8_t*)&otp_ctrl7);
    otp_ctrl7.otp_pwr_down = OTP_CTRL7_PWR_UP;

    status |= inv_imu_write_reg( OTP_CTRL7, 1, (uint8_t*)&otp_ctrl7);
    INV_LOG(INV_LOG_LEVEL_INFO, " otp w OTP_CTRL7 %x !!",otp_ctrl7);
    
	inv_delay_us(300);
    
	status |= inv_imu_read_reg(OTP_CTRL7, 1, (uint8_t*)&otp_ctrl7);
	otp_ctrl7.otp_reload = OTP_CTRL7_RELOAD_EN;
	status |= inv_imu_write_reg(OTP_CTRL7, 1, (uint8_t*)&otp_ctrl7);

	inv_delay_us(280);

	/*status |= inv_imu_switch_off_mclk();  */
	
	return status;
}

int inv_imu_device_reset(void)
{
	int status = INV_ERROR_SUCCESS;
	signal_path_reset_t signal_path_reset;
	int_status_t int_status;

	/* Reset the internal registers and restores the default settings.
	 * The bit automatically clears to 0 once the reset is done. */
	signal_path_reset.soft_reset_device_config = SIGNAL_PATH_RESET_SOFT_RESET_DEVICE_CONFIG_EN;
	status |= inv_imu_write_reg(SIGNAL_PATH_RESET, 1, (uint8_t*)&signal_path_reset);

	if (status)
		return status;

	/* Wait 1ms for soft reset to be effective before trying to perform any further read */
	inv_delay_ms(1);

    status |= configure_serial_interface();

	/* Clear the reset done int */
	status |= inv_imu_read_reg(INT_STATUS, 1, (uint8_t*)&int_status);
	if (int_status.reset_done_int!= INT_STATUS_RESET_DONE_INT_GENERATED) {
		status |= INV_ERROR_UNEXPECTED;
		return status;
	}

	/* Reload OTP procedure */
	status |= reload_otp();

	return status;
}

int inv_imu_set_timestamp_resolution(const TMST_CONFIG1_RESOL_t timestamp_resol)
{
	int status = 0;
	tmst_config1_t reg_tmst_config1;

	status |= inv_imu_read_reg(TMST_CONFIG1_MREG, 1, (uint8_t*)&reg_tmst_config1);
	reg_tmst_config1.tmst_res= timestamp_resol;
	status |= inv_imu_write_reg(TMST_CONFIG1_MREG, 1, (uint8_t*)&reg_tmst_config1);

	return status;
}

int inv_imu_configure_fifo_interface(void)
{
	int status = 0;
	int fifo_config;
	intf_config0_t reg_intf_config0;
	fifo_config1_t reg_fifo_config1;
	tmst_config1_t reg_tmst_config1;
	fifo_config5_t reg_fifo_config5;
	
	fifo_config = (icm_dev.fifo_is_used) ?INV_IMU_FIFO_ENABLED:INV_IMU_FIFO_ENABLED;
	inv_imu_switch_on_mclk();	

	status |= inv_imu_read_reg(INTF_CONFIG0, 1, (uint8_t*)&reg_intf_config0);
	reg_intf_config0.fifo_count_format  = INTF_CONFIG0_FIFO_COUNT_FORMAT_BYTE ;
	reg_intf_config0.fifo_count_endian  = INTF_CONFIG0_FIFO_COUNT_LITTLE_ENDIAN;
	reg_intf_config0.sensor_data_endian = INTF_CONFIG0_SENSOR_DATA_LITTLE_ENDIAN;
	status |= inv_imu_write_reg(INTF_CONFIG0, 1, (uint8_t*)&reg_intf_config0);

    status |= inv_imu_read_reg(FIFO_CONFIG1, 1, (uint8_t*)&reg_fifo_config1);
	reg_fifo_config1.fifo_bypass = FIFO_CONFIG1_FIFO_BYPASS_ON;
            
	switch (fifo_config) {
	
		case INV_IMU_FIFO_ENABLED :	
			/* Configure:
			  - FIFO record mode: snapshot mode i.e drop the data when the FIFO overflows
			  - Timestamp is logged in FIFO
			  - Little Endian fifo_count
			*/
		
			reg_fifo_config1.fifo_mode = FIFO_CONFIG1_FIFO_MODE_SNAPSHOT;	
			status |= inv_imu_write_reg(FIFO_CONFIG1, 1, (uint8_t*)&reg_fifo_config1);

            if(true == icm_dev.tmst_is_used){
			    status |= inv_imu_read_reg(TMST_CONFIG1_MREG, 1, (uint8_t*)&reg_tmst_config1);
			    reg_tmst_config1.tmst_en = TMST_CONFIG1_TMST_EN;
                reg_tmst_config1.tmst_fsyn_en = TMST_CONFIG1_TMST_FSYNC_DIS;
			    status |= inv_imu_write_reg(TMST_CONFIG1_MREG, 1, (uint8_t*)&reg_tmst_config1);
                INV_LOG(INV_LOG_LEVEL_INFO, "check fifo configure W TMST_CONFIG1_MREG  %x",reg_tmst_config1);
                
			}
			/* restart and reset FIFO configuration */
			status |= inv_imu_read_reg(FIFO_CONFIG5_MREG, 1, (uint8_t*)&reg_fifo_config5);
			
			reg_fifo_config5.fifo_gyro_en  = FIFO_CONFIG5_GYRO_EN;
			reg_fifo_config5.fifo_accel_en = FIFO_CONFIG5_ACCEL_EN ;
			reg_fifo_config5.fifo_tmst_fsync_en = FIFO_CONFIG5_TMST_FSYNC_DIS;
			reg_fifo_config5.fifo_wm_gt_th = FIFO_CONFIG5_WM_GT_TH_EN;

			if (icm_dev.fifo_highres_enabled)
			    reg_fifo_config5.fifo_hires_en = FIFO_CONFIG5_HIRES_EN;
            else
                reg_fifo_config5.fifo_hires_en = FIFO_CONFIG5_HIRES_DIS;
                
			status |= inv_imu_write_reg(FIFO_CONFIG5_MREG, 1, (uint8_t*)&reg_fifo_config5);
           

			break;

		
		case INV_IMU_FIFO_DISABLED :
			/* make sure FIFO is disabled */
		    /* restart and reset FIFO configuration */
			status |= inv_imu_read_reg(FIFO_CONFIG5_MREG, 1, (uint8_t*)&reg_fifo_config5);
			
			reg_fifo_config5.fifo_gyro_en  = FIFO_CONFIG5_GYRO_DIS;
			reg_fifo_config5.fifo_accel_en = FIFO_CONFIG5_ACCEL_DIS ;
			reg_fifo_config5.fifo_tmst_fsync_en = FIFO_CONFIG5_TMST_FSYNC_DIS;
            reg_fifo_config5.fifo_hires_en = FIFO_CONFIG5_HIRES_DIS;
                
			status |= inv_imu_write_reg(FIFO_CONFIG5_MREG, 1, (uint8_t*)&reg_fifo_config5);

            status |= inv_imu_write_reg(FIFO_CONFIG1, 1, (uint8_t*)&reg_fifo_config1);
			break;

		default :
			status = -1;
	}

	status |= inv_imu_switch_off_mclk();
	
	return status;
}


static int inv_imu_set_autorcosc_power_on(bool enable)
{
    int status = 0;
    fifo_config6_t reg_fifo_config6;
    /* Disable the automatic RCOSC power on to avoid extra power consumption in sleep mode (all sensors and clocks off) */
	status |= inv_imu_read_reg(FIFO_CONFIG6_MREG, 1, (uint8_t*)&reg_fifo_config6);
	if(enable == false)
	    reg_fifo_config6.rcosc_req_on_fifo_ths_dis = FIFO_CONFIG6_RCOSC_REQ_ON_FIFO_THS_DIS;
	else
	    reg_fifo_config6.rcosc_req_on_fifo_ths_dis = FIFO_CONFIG6_RCOSC_REQ_ON_FIFO_THS_EN;
	    
	status |= inv_imu_write_reg(FIFO_CONFIG6_MREG, 1, (uint8_t*)&reg_fifo_config6);

    return status;
}

static int inv_imu_get_convert(uint8_t direction, struct sensorConvert *cvt)
{
    struct sensorConvert *src;
    
    if (NULL == cvt)
        return -1;
    else if ( direction >= sizeof(inv_map) / sizeof(inv_map[0]) )
        return -1;

    src = &inv_map[direction];
    memcpy(cvt, src, sizeof(struct sensorConvert));
    
    return 0;
}

static int inv_imu_init_config(void)
{
    int status = 0;
    int_config_t reg_int_config;
    inv_imu_apex_parameters_t apex_inputs;
    
    INV_LOG(SENSOR_LOG_LEVEL, "%s", __func__);

    inv_delay_ms(3);
    
    status += configure_serial_interface();
    inv_delay_ms(3);
    status += inv_imu_device_reset();
    
    /* set BANK1 bus setting 
	  *change drive config for slew rate if necessary */

    /* Setup MEMs properties. */
	status |= inv_imu_read_reg(GYRO_CONFIG0, 1, (uint8_t*)&icm_dev.gyro_cfg0);
	status |= inv_imu_read_reg(ACCEL_CONFIG0, 1, (uint8_t*)&icm_dev.acc_cfg0);

	icm_dev.gyro_cfg0.gyro_ui_fs_sel = GYRO_CONFIG0_FS_SEL_2000dps;
	icm_dev.acc_cfg0.accel_ui_fs_sel = ACCEL_CONFIG0_FS_SEL_4g;
	status |= inv_imu_write_reg(GYRO_CONFIG0, 1, (uint8_t*)&icm_dev.gyro_cfg0);
	status |= inv_imu_write_reg(ACCEL_CONFIG0, 1, (uint8_t*)&icm_dev.acc_cfg0);

    /* Enable push pull on INT1 to avoid moving in Test Mode after a soft reset */
     /* set INT1 push-pull, active high,pulse mode */
	status |= inv_imu_read_reg(INT_CONFIG_REG, 1, (uint8_t*)&reg_int_config);

	reg_int_config.int1_drive_circuit = INT_CONFIG_INT1_DRIVE_CIRCUIT_PP; 
    reg_int_config.int1_polarity = INT_CONFIG_INT1_POLARITY_HIGH; 
    reg_int_config.int1_mode = INT_CONFIG_INT1_PULSE; 
	
	status |= inv_imu_write_reg(INT_CONFIG_REG, 1, (uint8_t*)&reg_int_config);

    //get inital int src reg value
    inv_imu_read_reg(INT_SOURCE0, 1, (uint8_t*)&icm_dev.int_src0);
	
   /* Set default timestamp resolution 16us (Mobile use cases) */
	status |= inv_imu_set_timestamp_resolution(TMST_CONFIG1_RESOL_16us);

    status |= inv_imu_configure_fifo_interface();
    status |= inv_imu_configure_wom(DEFAULT_WOM_THS_MG,
    DEFAULT_WOM_THS_MG,
    DEFAULT_WOM_THS_MG,
    WOM_CONFIG_WOM_INT_MODE_ANDED,
    WOM_CONFIG_WOM_INT_DUR_1_SMPL);

    //config APEX parameter
    status |= inv_imu_apex_init_parameters_struct(&apex_inputs);
    status |= inv_imu_apex_configure_parameters(&apex_inputs);
	
    status |= inv_imu_set_autorcosc_power_on(false);
 
    return status;
}

int inv_imu_initialize(void)
{
    int ret = 0;

    icm_dev.product = INVALID_TYPE;
    icm_dev.int_cfg = 0;
    icm_dev.init_cfg = false;
    icm_dev.dmp_power_save = false;
    icm_dev.apex_enable = false;
    
    icm_dev.dri_packet_size = 0;
    icm_dev.polling_data_en = false;
    icm_dev.fifo_is_used =  FIFO_WM_MODE_EN;
    icm_dev.fifo_highres_enabled = IS_HIGH_RES_MODE;
    icm_dev.tmst_is_used  = SUPPORT_FIFO_TS;
    icm_dev.serif_type = 0;
    icm_dev.pre_fifo_ts = 0;
    icm_dev.totol_sensor_ts = 0;
    icm_dev.min_apex_odr = SENSOR_HZ(25);

    icm_dev.sensors[PEDO].powered = false;
    icm_dev.sensors[WOM].powered = false;

    if(icm_dev.fifo_highres_enabled)
        icm_dev.fifo_packet_size = FIFO_20BYTES_PACKET_SIZE;
    else
        icm_dev.fifo_packet_size = FIFO_16BYTES_PACKET_SIZE;

    INV_LOG(SENSOR_LOG_LEVEL, "inv_imu_initialize");
    configure_serial_interface();
    inv_imu_get_whoami();
    
    if (icm_dev.product == INVALID_TYPE)
        return SENSOR_WHOAMI_INVALID_ERROR;

    ret += inv_imu_get_convert(SENSOR_DIRECTION, &icm_dev.cvt);
    if (ret != 0)
        return SENSOR_CONVERT_INVALID_ERROR;

    INV_LOG(SENSOR_LOG_LEVEL, "sensor axis[0]:%d, axis[1]:%d, axis[2]:%d, sign[0]:%d, sign[1]:%d, sign[2]:%d",
          icm_dev.cvt.axis[AXIS_X], icm_dev.cvt.axis[AXIS_Y], icm_dev.cvt.axis[AXIS_Z],
          icm_dev.cvt.sign[AXIS_X], icm_dev.cvt.sign[AXIS_Y], icm_dev.cvt.sign[AXIS_Z]);
    
    ret += inv_imu_init_config();
  
    if (ret == 0) {
        icm_dev.init_cfg = true;
        INV_LOG(SENSOR_LOG_LEVEL, "Initialize Success");
    } else {
        INV_LOG(SENSOR_LOG_LEVEL, "Initialize Failed %d", ret);
        ret = SENSOR_INIT_INVALID_ERROR;
    }

    return ret;
}

int inv_imu_enable_accel_low_power_mode(uint32_t accel_rate_us )
{
	int status = 0;
    PWR_MGMT_0_ACCEL_MODE_t accel_mode;
    PWR_MGMT_0_GYRO_MODE_t  gyro_mode;
    
	status |= inv_imu_read_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);
    accel_mode = (PWR_MGMT_0_ACCEL_MODE_t) icm_dev.pwr_mgmt.accel_mode; 
    gyro_mode  = (PWR_MGMT_0_GYRO_MODE_t ) icm_dev.pwr_mgmt.gyro_mode; 

    if (accel_rate_us == 0){
        accel_rate_us = 80000;  // 80 ms for enough safe time ;
    }
    
	/* Check if the accelerometer is the only one enabled */
	if( (accel_mode != PWR_MGMT_0_ACCEL_MODE_LP) && 
	    ((gyro_mode == PWR_MGMT_0_GYRO_MODE_OFF) || 
	     (gyro_mode == PWR_MGMT_0_GYRO_MODE_STANDBY)) ){
		/* Select the RC OSC as clock source for the accelerometer */
		status |= select_rcosc();
	}

    /* Enable/Switch the accelerometer in/to low power mode */
    status |= inv_imu_read_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);
	icm_dev.pwr_mgmt.accel_mode = PWR_MGMT_0_ACCEL_MODE_LP;
	status |= inv_imu_write_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);
	inv_delay_us(200);  
#if 0
	if( (accel_mode != PWR_MGMT_0_ACCEL_MODE_LP) && 
	    (gyro_mode == PWR_MGMT_0_GYRO_MODE_OFF) || 
	     (gyro_mode == PWR_MGMT_0_GYRO_MODE_STANDBY)) ){
		/* Wait one accelerometer ODR before switching to the WU OSC */	
        if(accel_rate_us>=1000)
            inv_delay_ms(accel_rate_us/1000+1);
        else
		    inv_delay_us(accel_rate_us);
        
		status |= select_wuosc();
	}
#endif
	
	/* Enable the automatic RCOSC power on so that FIFO is entirely powered on */
	status |= inv_imu_set_autorcosc_power_on(true);
	return status;
}

int inv_imu_enable_accel_low_noise_mode(uint32_t accel_rate_us)
{
	int status = 0;
	PWR_MGMT_0_ACCEL_MODE_t accel_mode;
    PWR_MGMT_0_GYRO_MODE_t  gyro_mode;

    if (accel_rate_us == 0 )
         accel_rate_us = 80000 ;  // 80 ms for enough safe time ;
         
	status |= inv_imu_read_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);
    accel_mode = (PWR_MGMT_0_ACCEL_MODE_t) icm_dev.pwr_mgmt.accel_mode; 
    gyro_mode = (PWR_MGMT_0_GYRO_MODE_t) icm_dev.pwr_mgmt.gyro_mode; 
    
	/* Check if the accelerometer is the only one enabled */
	if ((accel_mode == PWR_MGMT_0_ACCEL_MODE_LP) && 
	    ((gyro_mode == PWR_MGMT_0_GYRO_MODE_OFF) || (gyro_mode == PWR_MGMT_0_GYRO_MODE_STANDBY))){		
		
		/* Select the RC OSC as clock source for the accelerometer */
		status |= select_rcosc();
		/* Wait one accel ODR before switching to low noise mode */
        if(accel_rate_us>=1000)
            inv_delay_ms(accel_rate_us/1000+1);
        else
		    inv_delay_us(accel_rate_us);
	}

	/* Enable/Switch the accelerometer in/to low noise mode */
	status |= inv_imu_read_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);
	icm_dev.pwr_mgmt.accel_mode = PWR_MGMT_0_ACCEL_MODE_LN;
    status |= inv_imu_write_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);

    /* Disable the automatic RCOSC power on so that FIFO is entirely powered on */
	status |= inv_imu_set_autorcosc_power_on(false);
	inv_delay_us(200);  

	return status;
}

static int inv_imu_set_odr(int index)
{
    int ret = 0;
    uint8_t regValue = 0;
    
    regValue = IMU_ODR_MAPPING[index];
    INV_LOG(SENSOR_LOG_LEVEL, "Odr Reg value 0x%x", regValue);
    /* update new odr */
    icm_dev.gyro_cfg0.gyro_odr = regValue;
    ret |= inv_imu_write_reg(GYRO_CONFIG0, 1, (uint8_t*)&icm_dev.gyro_cfg0);
    INV_LOG(SENSOR_LOG_LEVEL, "write GYRO_CONFIG0  0x%x", icm_dev.gyro_cfg0);    
    icm_dev.acc_cfg0.accel_odr = regValue;
    ret |= inv_imu_write_reg(ACCEL_CONFIG0,1, (uint8_t*)&icm_dev.acc_cfg0);
    INV_LOG(SENSOR_LOG_LEVEL, "write ACCEL_CONFIG0  0x%x", icm_dev.gyro_cfg0); 
 
    return ret;
}

static int inv_imu_cal_odr(uint32_t *rate, uint32_t *report_rate)
{
    uint8_t i;

    for (i = 0; i < (ARRAY_SIZE(IMUHWRates)); i++) {
        if (*rate <= IMUHWRates[i]) {
            *report_rate = IMUHWRates[i];
            break;
        }
    }

    if (*rate > IMUHWRates[(ARRAY_SIZE(IMUHWRates) - 1)]) {
        i = (ARRAY_SIZE(IMUHWRates) - 1);
        *report_rate = IMUHWRates[i];
    }

    return (int)i;
}

/* return watermark in bytes with input as number of packets */
static uint16_t inv_imu_cal_wm(uint16_t packet)
{
    uint8_t min_watermark = 1;
    uint8_t max_watermark ;
    uint16_t real_watermark = 0;   //watermark in packet (record) number
    
    // For Xian the max FIFO size is 2048 Bytes, common FIFO package 16 Bytes, so the max number of FIFO pakage is 128.
    max_watermark = MAX_RECV_PACKET/2 ; /*64*/

    real_watermark = packet;
    real_watermark = real_watermark < min_watermark ? min_watermark : real_watermark;
    real_watermark = real_watermark > max_watermark ? max_watermark : real_watermark;
    
    return real_watermark * icm_dev.fifo_packet_size;  /* byte mode*/
}

int inv_imu_reset_fifo(void)
{
	int status = 0;
	signal_path_reset_t fifo_flush_status;
	status |= inv_imu_read_reg(SIGNAL_PATH_RESET, 1,(uint8_t*) &fifo_flush_status);
	
	fifo_flush_status.fifo_flush = 1;
	
	status |= inv_imu_switch_on_mclk();

	status |= inv_imu_write_reg(SIGNAL_PATH_RESET, 1, (uint8_t*)&fifo_flush_status);
	inv_delay_us(10);
	
	/* Wait for FIFO flush (idle bit will go high at appropriate time and unlock flush) */
	while( (1==fifo_flush_status.fifo_flush) && (0==status) )
	{
		status |= inv_imu_read_reg(SIGNAL_PATH_RESET, 1, (uint8_t*)&fifo_flush_status);
		inv_delay_us(2);
	}
	
	status |= inv_imu_switch_off_mclk();

	return status;
}

static int inv_imu_read_fifo(void)
{
    int ret = 0;
    uint8_t count[2];
    ret |= inv_imu_switch_on_mclk();

    /* FIFO byte mode configured at driver init, so we read byte number, not pakage count */
    if((ret |= inv_imu_read_reg(FIFO_COUNTH, 2, &count[0])) != INV_ERROR_SUCCESS){
	    ret += inv_imu_switch_off_mclk();
		return ret;
	}
    icm_dev.fifoDataToRead = (((uint16_t)count[1]) <<8) | count[0];

#if !SENSOR_LOG_TS_ONLY
    INV_LOG(SENSOR_LOG_LEVEL, "Fifo count is %d bytes", icm_dev.fifoDataToRead);
#endif
    if (icm_dev.fifoDataToRead <= 0 || icm_dev.fifoDataToRead > IMU_MAX_FIFO_SIZE)
        return FIFO_COUNT_INVALID_ERROR;

    ret += inv_imu_read_reg(FIFO_DATA, icm_dev.fifoDataToRead, icm_dev.dataBuf);

    ret += inv_imu_switch_off_mclk();
    INV_LOG(SENSOR_LOG_LEVEL, "read Fifo end");

    return ret;
}

static int inv_imu_config_fifo(bool enable)
{
    int ret = 0;
    fifo_config1_t reg_fifo_config1;
    uint8_t  buffer[2],data;
    uint16_t watermarkReg;

    watermarkReg = icm_dev.watermark;

    if (watermarkReg < icm_dev.fifo_packet_size)
    watermarkReg = icm_dev.fifo_packet_size;
    
    INV_LOG(SENSOR_LOG_LEVEL, "%s watermarkReg %d", __func__, watermarkReg);

    buffer[0] = watermarkReg & 0x00FF;
    buffer[1] = (watermarkReg & 0xFF00) >> 8;
    
    ret |= inv_imu_switch_on_mclk();

    ret |= inv_imu_read_reg(FIFO_CONFIG1, 1, (uint8_t*)&reg_fifo_config1);
    if (enable) {
         /* fifo mode, set by pass on */

        reg_fifo_config1.fifo_mode = FIFO_CONFIG1_FIFO_MODE_SNAPSHOT;
        reg_fifo_config1.fifo_bypass = FIFO_CONFIG1_FIFO_BYPASS_ON;
        ret |=inv_imu_write_reg(FIFO_CONFIG1,1,(uint8_t*)&reg_fifo_config1 );
        /* flush fifo */
		data = (uint8_t)SIGNAL_PATH_RESET_FIFO_FLUSH_EN;
        ret |=inv_imu_write_reg(SIGNAL_PATH_RESET,1,&data);

        inv_delay_ms(10);
        
        /* set threshold */
        ret |=inv_imu_write_reg(FIFO_CONFIG2,1, &buffer[0]);
        ret |=inv_imu_write_reg(FIFO_CONFIG3,1, &buffer[1]);
        
        if (icm_dev.sensors[ACC].configed || icm_dev.sensors[GYR].configed) {
            /* set fifo stream mode */
           reg_fifo_config1.fifo_mode = FIFO_CONFIG1_FIFO_MODE_SNAPSHOT;
           reg_fifo_config1.fifo_bypass = FIFO_CONFIG1_FIFO_BYPASS_OFF;
           ret |=inv_imu_write_reg(FIFO_CONFIG1,1,(uint8_t*)&reg_fifo_config1 );
        }
        INV_LOG(SENSOR_LOG_LEVEL, "ConfigFifo: Reset, TH_L:0x%x, TH_H:0x%x\n",
            buffer[0], buffer[1]);
    } else {
        ret |=inv_imu_write_reg(FIFO_CONFIG2,1, &buffer[0]);
        ret |=inv_imu_write_reg(FIFO_CONFIG3,1, &buffer[1]);

        reg_fifo_config1.fifo_mode = FIFO_CONFIG1_FIFO_MODE_SNAPSHOT;
        
        if (icm_dev.sensors[ACC].configed || icm_dev.sensors[GYR].configed) {
            reg_fifo_config1.fifo_bypass = FIFO_CONFIG1_FIFO_BYPASS_OFF;
        } else {
            reg_fifo_config1.fifo_bypass = FIFO_CONFIG1_FIFO_BYPASS_ON;
        }

        ret |=inv_imu_write_reg(FIFO_CONFIG1,1,(uint8_t*)&reg_fifo_config1 );
        INV_LOG(SENSOR_LOG_LEVEL, "ConfigFifo, TH_L:0x%x, TH_H:0x%x\n",
            buffer[0], buffer[1]);

        /*to do if necessary , add fifo disable for gyro& accel in FIFO_CONFIG5_MREG */
    }

    inv_imu_switch_off_mclk();

    return ret;
}

int inv_imu_enable_high_resolution_fifo(void)
{
    fifo_config5_t reg_fifo_config5;
	int status = 0;

	/* set FIFO packets to 20bit format (i.e. high res is enabled) */

	status |= inv_imu_read_reg(FIFO_CONFIG5_MREG, 1, (uint8_t*)&reg_fifo_config5);
	reg_fifo_config5.fifo_hires_en = FIFO_CONFIG5_HIRES_EN;
	status |= inv_imu_write_reg(FIFO_CONFIG5_MREG, 1, (uint8_t*)&reg_fifo_config5);

	return status;
}

int inv_imu_disable_high_resolution_fifo(void)
{
    fifo_config5_t reg_fifo_config5;
	int status = 0;

	status |= inv_imu_read_reg(FIFO_CONFIG5_MREG, 1, (uint8_t*)&reg_fifo_config5);
    reg_fifo_config5.fifo_hires_en = FIFO_CONFIG5_HIRES_DIS;
	status |= inv_imu_write_reg(FIFO_CONFIG5_MREG, 1, (uint8_t*)&reg_fifo_config5);

	return status;
}

int inv_imu_config_drdy(bool enable)
{
    int ret = 0;

    if (enable &&
        (true == icm_dev.sensors[ACC].configed ||
        true == icm_dev.sensors[GYR].configed)) {
        if (!icm_dev.int_src0.drdy_int1_en) {
            icm_dev.int_src0.drdy_int1_en = 1;
            ret += inv_imu_write_reg(INT_SOURCE0,1,(uint8_t*)&icm_dev.int_src0);
            INV_LOG(SENSOR_LOG_LEVEL, "Enable DRDY INT1 0x%x", icm_dev.int_src0);
        }
    } else {
        icm_dev.int_src0.drdy_int1_en = 0;
        ret += inv_imu_write_reg(INT_SOURCE0,1,(uint8_t*)&icm_dev.int_src0);
        INV_LOG(SENSOR_LOG_LEVEL, "Disable DRDY INT1 0x%x", icm_dev.int_src0);
    }

    return ret;
}

int inv_imu_config_fifo_int(bool enable)
{
    int ret = 0;

    if (enable &&
        (true == icm_dev.sensors[ACC].configed ||
         true == icm_dev.sensors[GYR].configed)) {
        if ( !icm_dev.int_src0.fifo_ths_int1_en ) {
            icm_dev.int_src0.fifo_ths_int1_en = 1;
            ret += inv_imu_write_reg(INT_SOURCE0,1,(uint8_t*)&icm_dev.int_src0);
            INV_LOG(SENSOR_LOG_LEVEL, "Enable FIFO WM INT1 0x%x", icm_dev.int_src0);
        }
    } else {
        icm_dev.int_src0.fifo_ths_int1_en = 0;
        ret += inv_imu_write_reg(INT_SOURCE0,1,(uint8_t*)&icm_dev.int_src0);
        INV_LOG(SENSOR_LOG_LEVEL, "Disable FIFO WM INT1 0x%x", icm_dev.int_src0);
    }

    return ret;
}

int inv_imu_config_fifofull_int(bool enable)
{
    int ret = 0;

    if (enable &&
        (true == icm_dev.sensors[ACC].configed ||
         true == icm_dev.sensors[GYR].configed)) {
        if (!icm_dev.int_src0.fifo_full_int1_en) {
            icm_dev.int_src0.fifo_full_int1_en =1 ;
            ret += inv_imu_write_reg(INT_SOURCE0,1,(uint8_t*)&icm_dev.int_src0);
            INV_LOG(SENSOR_LOG_LEVEL, "Enable FIFO WM INT1 0x%x", icm_dev.int_src0);
        }
    } else {
        icm_dev.int_src0.fifo_full_int1_en = 0 ;
        ret += inv_imu_write_reg(INT_SOURCE0,1,(uint8_t*)&icm_dev.int_src0);
        INV_LOG(SENSOR_LOG_LEVEL, "Disable FIFO WM INT1 0x%x", icm_dev.int_src0);
    }

    return ret;
}

int inv_imu_set_accel_fsr(ACCEL_CONFIG0_FS_SEL_t accel_fsr_g)
{
	int status = 0;

    status |= inv_imu_read_reg(ACCEL_CONFIG0, 1, (uint8_t*)&icm_dev.acc_cfg0);

    if((icm_dev.fifo_highres_enabled) && (icm_dev.fifo_is_used == INV_IMU_FIFO_ENABLED))
        icm_dev.acc_cfg0.accel_ui_fs_sel = ACCEL_CONFIG0_FS_SEL_MAX;
    else{
        icm_dev.acc_cfg0.accel_ui_fs_sel = accel_fsr_g;
	}
	
	status |= inv_imu_write_reg(ACCEL_CONFIG0, 1, (uint8_t*)&icm_dev.acc_cfg0);
 
	return status;
}

int inv_imu_set_gyro_fsr(GYRO_CONFIG0_FS_SEL_t gyro_fsr_dps)
{
	int status = 0;

	status |= inv_imu_read_reg(GYRO_CONFIG0, 1, (uint8_t*)&icm_dev.gyro_cfg0);

    if((icm_dev.fifo_highres_enabled) && (icm_dev.fifo_is_used == INV_IMU_FIFO_ENABLED))
        icm_dev.gyro_cfg0.gyro_ui_fs_sel = GYRO_CONFIG0_FS_SEL_MAX; 
    else{
        icm_dev.gyro_cfg0.gyro_ui_fs_sel = gyro_fsr_dps;
    }
    
	status |= inv_imu_write_reg(GYRO_CONFIG0, 1, (uint8_t*)&icm_dev.gyro_cfg0);
  
	return status;
}

int inv_imu_acc_enable(void)
{
    int ret = 0;
    
    ret |= inv_imu_read_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);
    icm_dev.sensors[ACC].powered = true;
    icm_dev.pwr_mgmt.accel_mode = PWR_MGMT_0_ACCEL_MODE_LN;
    ret |= inv_imu_write_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);
    
    INV_LOG(SENSOR_LOG_LEVEL, "accel Enable power mode=%d, pwr %x \n", icm_dev.sensors[ACC].powered, icm_dev.pwr_mgmt);

     /* Enable the automatic RCOSC power on so that FIFO is entirely powered on */
	//ret |= inv_imu_set_autorcosc_power_on(true);
    // set 20ms for accel start-up time
    inv_delay_ms(20);
    
    return ret;
}

int inv_imu_acc_disable()
{
    int ret = 0;
    int odr_index = 0;
    uint32_t sampleRate = 0;
    bool accelOdrChanged = false;
    bool watermarkChanged = false;

    INV_LOG(SENSOR_LOG_LEVEL, "%s", __func__);
    
    icm_dev.sensors[ACC].preRealRate = 0;
    icm_dev.sensors[ACC].hwRate = 0;
    icm_dev.sensors[ACC].needDiscardSample = false;
    icm_dev.sensors[ACC].samplesToDiscard = 0;
    icm_dev.sensors[ACC].wm = 0;

    //get new pwr_mgmt reg value 
    ret |= inv_imu_read_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);

    if ((false == icm_dev.sensors[GYR].powered) 
     && (false == icm_dev.sensors[PEDO].powered)  
     && (false == icm_dev.sensors[WOM].powered)) 
    {
        inv_imu_switch_off_mclk();
        /* turn off acc & gyro */
        icm_dev.pwr_mgmt.accel_mode = PWR_MGMT_0_ACCEL_MODE_OFF;
        icm_dev.pwr_mgmt.gyro_mode = PWR_MGMT_0_GYRO_MODE_OFF;
        ret |= inv_imu_write_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);

        inv_delay_us(200); //spec: 200us
        
        INV_LOG(SENSOR_LOG_LEVEL, "acc off pwr: 0x%x", icm_dev.pwr_mgmt);
    } else if (true == icm_dev.sensors[GYR].powered) {  //  update gyro old odr
        if (icm_dev.sensors[GYR].hwRate != icm_dev.sensors[GYR].preRealRate) {
            icm_dev.sensors[GYR].hwRate = icm_dev.sensors[GYR].preRealRate;
            odr_index = inv_imu_cal_odr(&icm_dev.sensors[GYR].hwRate, &sampleRate);
            ret += inv_imu_set_odr(odr_index);
            INV_LOG(SENSOR_LOG_LEVEL, "gyro revert rate to preRealRate: %f Hz", 
                  (icm_dev.sensors[GYR].hwRate/1024.0f));
            accelOdrChanged = true;
        }
        if (icm_dev.fifo_is_used && (icm_dev.watermark != icm_dev.sensors[GYR].wm)) {
            icm_dev.watermark = icm_dev.sensors[GYR].wm;
            watermarkChanged = true;
            INV_LOG(SENSOR_LOG_LEVEL, "watermark revert to: %d", icm_dev.watermark/icm_dev.fifo_packet_size);
        }
            
        if ((false == icm_dev.sensors[PEDO].powered)
            && (false == icm_dev.sensors[WOM].powered)
            ) {
            icm_dev.pwr_mgmt.accel_mode = PWR_MGMT_0_ACCEL_MODE_OFF;
            ret |= inv_imu_write_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);
            inv_delay_us(200); //spec: 200us

            INV_LOG(SENSOR_LOG_LEVEL, "gyro on and acc off pwr: 0x%x", icm_dev.pwr_mgmt);
        } else {
            INV_LOG(SENSOR_LOG_LEVEL, "gyro on and apex on pwr: 0x%x", icm_dev.pwr_mgmt);
        }
    }
    else if((true == icm_dev.sensors[PEDO].powered)
            ||(true == icm_dev.sensors[WOM].powered))
    {
        if(icm_dev.sensors[ACC].hwRate!=0)
            inv_imu_enable_accel_low_power_mode(1024000000 / icm_dev.sensors[ACC].hwRate);
        else
            inv_imu_enable_accel_low_power_mode(0);
    }
    icm_dev.sensors[ACC].powered = false;
    icm_dev.sensors[ACC].configed = false;

    if (true == icm_dev.fifo_is_used) {
        inv_imu_config_fifo(accelOdrChanged | watermarkChanged);
    } 

    if ((false == icm_dev.sensors[GYR].powered) 
     && (false == icm_dev.sensors[ACC].powered)){
        //disable fifo rcosc 
        inv_imu_set_autorcosc_power_on(false);
        
        // disable interruput
        if (true == icm_dev.fifo_is_used) {
            inv_imu_config_fifo_int(false);
            inv_imu_config_fifofull_int(false);
        } else {
            inv_imu_config_drdy(false);
        }
     }
     
    #if 0
    if (accelOdrChanged | watermarkChanged) {
        //do nothing
    }
    #endif
    
    return ret;
}

int inv_imu_gyro_enable()
{
    int ret = 0;

    INV_LOG(SENSOR_LOG_LEVEL, "%s", __func__);

    ret |= inv_imu_read_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);
    icm_dev.sensors[GYR].powered = true;
    icm_dev.pwr_mgmt.gyro_mode = PWR_MGMT_0_GYRO_MODE_LN;
    ret |= inv_imu_write_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);

    //inv_imu_set_autorcosc_power_on(true);
    
    //100ms here to discard invalid data
    inv_delay_ms(100);
    INV_LOG(SENSOR_LOG_LEVEL, "GYR PWR 0x%x", icm_dev.pwr_mgmt);
    
    return ret;
}

int inv_imu_gyro_disable()
{
    int ret = 0;
    int odr_index = 0;
    uint32_t sampleRate = 0;
    bool gyroOdrChanged = false;
    bool watermarkChanged = false;

    INV_LOG(SENSOR_LOG_LEVEL, "%s", __func__);
    
    icm_dev.sensors[GYR].preRealRate = 0;
    icm_dev.sensors[GYR].hwRate = 0;
    icm_dev.sensors[GYR].needDiscardSample = false;
    icm_dev.sensors[GYR].samplesToDiscard = 0;
    icm_dev.sensors[GYR].wm = 0;

    //get new pwr_mgmt reg value 
    ret |= inv_imu_read_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);
    
    if (true == icm_dev.sensors[ACC].powered) {  //  update ACC old ord
        if(icm_dev.sensors[ACC].hwRate != icm_dev.sensors[ACC].preRealRate) {
            icm_dev.sensors[ACC].hwRate = icm_dev.sensors[ACC].preRealRate;
            odr_index = inv_imu_cal_odr(&icm_dev.sensors[ACC].hwRate, &sampleRate);
            ret += inv_imu_set_odr(odr_index);
            gyroOdrChanged = true;
            INV_LOG(SENSOR_LOG_LEVEL, "acc revert rate to preRealRate: %f Hz",
                (icm_dev.sensors[ACC].hwRate/1024.0f));
            if (icm_dev.fifo_is_used && (icm_dev.watermark != icm_dev.sensors[ACC].wm)) {
                icm_dev.watermark = icm_dev.sensors[ACC].wm;
                watermarkChanged = true;
                INV_LOG(SENSOR_LOG_LEVEL, "watermark revert to: %d", icm_dev.watermark/icm_dev.fifo_packet_size);
            }
        }

		odr_index = inv_imu_cal_odr(&icm_dev.sensors[ACC].hwRate, &sampleRate);
		
	    if(odr_index <2)    
            inv_imu_enable_accel_low_power_mode(1024000000 / icm_dev.sensors[ACC].hwRate);
        else
           	inv_imu_enable_accel_low_noise_mode(1024000000 / icm_dev.sensors[ACC].hwRate);
		
        icm_dev.pwr_mgmt.gyro_mode = PWR_MGMT_0_GYRO_MODE_OFF;
        ret += inv_imu_write_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);

        inv_delay_ms(20); // for 20ms gyro ringdwon
        INV_LOG(SENSOR_LOG_LEVEL, "acc on and gyro off pwr: 0x%x", icm_dev.pwr_mgmt);
    }else if((icm_dev.sensors[WOM].powered == true) ||(icm_dev.sensors[PEDO].powered == true)){

        icm_dev.pwr_mgmt.gyro_mode = PWR_MGMT_0_GYRO_MODE_OFF;
        ret += inv_imu_write_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);
        
        inv_imu_enable_accel_low_power_mode(0);
    }else {
        /* Gyro OFF & Accel off*/
        icm_dev.pwr_mgmt.accel_mode = PWR_MGMT_0_ACCEL_MODE_OFF;
        icm_dev.pwr_mgmt.gyro_mode = PWR_MGMT_0_GYRO_MODE_OFF;
        ret |= inv_imu_write_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);

        inv_delay_ms(20);   // for 20ms gyro ringdwon
        INV_LOG(SENSOR_LOG_LEVEL, "gyro off pwr: 0x%x", icm_dev.pwr_mgmt);
    }
    
    icm_dev.sensors[GYR].powered = false;
    icm_dev.sensors[GYR].configed = false;

    if (true == icm_dev.fifo_is_used) {
        inv_imu_config_fifo(gyroOdrChanged | watermarkChanged);
    } 

    if ((false == icm_dev.sensors[GYR].powered) 
    && (false == icm_dev.sensors[ACC].powered)) {
        //disable fifo rcosc 
        inv_imu_set_autorcosc_power_on(false);
        
        // disable interruput
        if (true == icm_dev.fifo_is_used) {
            inv_imu_config_fifo_int(false);
            inv_imu_config_fifofull_int(false);
        } else {
            inv_imu_config_drdy(false);
        }
    }
     
    #if 0
    if (gyroOdrChanged | watermarkChanged) {
        //do nothing
    }
    #endif
    
    return ret;
}

int inv_imu_acc_set_rate(float odr_hz, uint16_t packet_num,float *hw_odr)
{
    int ret = 0;
    int odr_index = 0;
    uint32_t sampleRate = 0;
    uint32_t maxRate = 0;
    bool accelOdrChanged = false;
    bool watermarkChanged = false;

    INV_LOG(SENSOR_LOG_LEVEL, "%s odr_hz %f package num setting for wm %d", __func__, odr_hz, packet_num);
    icm_dev.sensors[ACC].rate = SENSOR_HZ(odr_hz);
    
    if ((true == icm_dev.apex_enable) &&
        (icm_dev.sensors[ACC].rate < icm_dev.min_apex_odr)) {
        INV_LOG(SENSOR_LOG_LEVEL, "APEX Enabled, Acc min odr to 50Hz!!");
        icm_dev.sensors[ACC].rate = icm_dev.min_apex_odr;  
    }
    
    if (icm_dev.sensors[ACC].preRealRate == 0) {
        icm_dev.sensors[ACC].needDiscardSample = true;
        icm_dev.sensors[ACC].samplesToDiscard = NUM_TODISCARD;
    }
    
    odr_index = inv_imu_cal_odr(&icm_dev.sensors[ACC].rate, &sampleRate);
    icm_dev.sensors[ACC].preRealRate = sampleRate;

    /* if gyr configed ,compare maxRate with acc and gyr rate */
    if (true == icm_dev.sensors[GYR].configed) {
        maxRate = max(sampleRate, icm_dev.sensors[GYR].preRealRate);// choose with preRealRate
        if (maxRate != icm_dev.sensors[ACC].hwRate ||
            maxRate != icm_dev.sensors[GYR].hwRate) {
            icm_dev.sensors[ACC].hwRate = maxRate;
            icm_dev.sensors[GYR].hwRate = maxRate;
            INV_LOG(SENSOR_LOG_LEVEL, "New Acc/Gyro config Rate %f Hz",
                (icm_dev.sensors[ACC].hwRate/1024.0f));
            odr_index = inv_imu_cal_odr(&maxRate, &sampleRate);
            ret += inv_imu_set_odr(odr_index);
            accelOdrChanged = true;
        } else
            accelOdrChanged = false;
    } else {
        if ((sampleRate != icm_dev.sensors[ACC].hwRate)) {
            icm_dev.sensors[ACC].hwRate = sampleRate;
            INV_LOG(SENSOR_LOG_LEVEL, "New Acc config Rate %f Hz",
                (icm_dev.sensors[ACC].hwRate/1024.0f));
            ret += inv_imu_set_odr(odr_index);
            /*enable ACCEL LP mode when odr < 50hz + only Accel on */ 
            if(odr_index <2)    
                inv_imu_enable_accel_low_power_mode(1024000000 / icm_dev.sensors[ACC].hwRate);
            else
           	    inv_imu_enable_accel_low_noise_mode(1024000000 / icm_dev.sensors[ACC].hwRate);    
            accelOdrChanged = true;
        } else
            accelOdrChanged = false;
    }
    icm_dev.sensors[ACC].configed = true;
	*hw_odr = icm_dev.sensors[ACC].hwRate / 1024 ;
    //For fifo mode
    if (true == icm_dev.fifo_is_used) {
        icm_dev.sensors[ACC].wm = inv_imu_cal_wm(packet_num);
        if (icm_dev.sensors[ACC].wm != icm_dev.watermark) {
            watermarkChanged = true;
            icm_dev.watermark = icm_dev.sensors[ACC].wm;
            INV_LOG(SENSOR_LOG_LEVEL, "New package num for watermark is %d", icm_dev.watermark/icm_dev.fifo_packet_size);
        } else {
            watermarkChanged = false;
        }
        inv_imu_config_fifo(accelOdrChanged | watermarkChanged);
    } 

    // config interruput
    if (true == icm_dev.fifo_is_used) {
        inv_imu_config_fifo_int(true);
        inv_imu_config_fifofull_int(true);
    } else {
        inv_imu_config_drdy(true);
    }

#if SENSOR_REG_DUMP
    inv_imu_dumpRegs();
#endif

    return ret;
}

int inv_imu_gyro_set_rate(float odr_hz, uint16_t packet_num,float *hw_odr)
{
    int ret = 0;
    int odr_index = 0;
    uint32_t sampleRate = 0;
    uint32_t maxRate = 0;
    bool gyroOdrChanged = false;
    bool watermarkChanged = false;
    
    INV_LOG(SENSOR_LOG_LEVEL, "%s odr_hz %f packnum %d for setting wm", __func__, odr_hz, packet_num);
    icm_dev.sensors[GYR].rate = SENSOR_HZ(odr_hz);

    if (icm_dev.sensors[GYR].preRealRate == 0) {
        icm_dev.sensors[GYR].needDiscardSample = true;
        icm_dev.sensors[GYR].samplesToDiscard = NUM_TODISCARD;
    }

    /* get hw sample rate */
    odr_index = inv_imu_cal_odr(&icm_dev.sensors[GYR].rate, &sampleRate);

    icm_dev.sensors[GYR].preRealRate = sampleRate;

    /* if acc configed ,compare maxRate with acc and gyr rate */
    if (true == icm_dev.sensors[ACC].configed) {
        maxRate = max(sampleRate, icm_dev.sensors[ACC].preRealRate);
        if (maxRate != icm_dev.sensors[ACC].hwRate ||
            maxRate != icm_dev.sensors[GYR].hwRate) {
            icm_dev.sensors[ACC].hwRate = maxRate;
            icm_dev.sensors[GYR].hwRate = maxRate;
            INV_LOG(SENSOR_LOG_LEVEL, "New Gyro/Acc config Rate %f Hz",
                (icm_dev.sensors[GYR].hwRate/1024.0f));
            /* update new odr */
            odr_index = inv_imu_cal_odr(&maxRate, &sampleRate);
            ret += inv_imu_set_odr(odr_index);
            gyroOdrChanged = true;
        } else
            gyroOdrChanged = false;

          //confirm accel back to LN mode
        inv_imu_enable_accel_low_noise_mode(1024000000 / icm_dev.sensors[ACC].hwRate);     
    } else {
        if ((sampleRate != icm_dev.sensors[GYR].hwRate)) {
            icm_dev.sensors[GYR].hwRate = sampleRate;
            INV_LOG(SENSOR_LOG_LEVEL, "New Gyro config Rate %f Hz",
                (icm_dev.sensors[GYR].hwRate/1024.0f));
            /* update new odr */
            ret += inv_imu_set_odr(odr_index);
            gyroOdrChanged = true;
        } else
            gyroOdrChanged = false;
    }

    icm_dev.sensors[GYR].configed = true;

	*hw_odr = icm_dev.sensors[GYR].hwRate / 1024 ;
		
    //For fifo mode
    if (true == icm_dev.fifo_is_used) {
        icm_dev.sensors[GYR].wm = inv_imu_cal_wm(packet_num);
        if (icm_dev.sensors[GYR].wm != icm_dev.watermark) {
            watermarkChanged = true;
            icm_dev.watermark = icm_dev.sensors[GYR].wm;
            INV_LOG(SENSOR_LOG_LEVEL, "New package number for Watermark is %d", icm_dev.watermark/icm_dev.fifo_packet_size);
        } else {
            watermarkChanged = false;
        }
        inv_imu_config_fifo(gyroOdrChanged | watermarkChanged);
    } 

   
     // config interruput
    if (true == icm_dev.fifo_is_used) {
        inv_imu_config_fifo_int(true);
        inv_imu_config_fifofull_int(true);
    } else {
        inv_imu_config_drdy(true);
    }
    
    #if 0
    if(gyroOdrChanged || watermarkChanged)
    {
    
        //* Clear the interrupt */
        //do nothing right now
    }
    #endif

#if SENSOR_REG_DUMP
    inv_imu_dumpRegs();
#endif

    return ret;
}

static void inv_imu_parse_rawdata(uint8_t *buf, uint8_t cur_index,SensorType_t sensorType, struct accGyroData *data)
{
	uint8_t high_res_rel_addr=0;
    uint16_t cur_fifo_ts = 0;
	int i=0;

	// Output raw A+G data.	
	if( TEMP==sensorType ) 
	{
		if (true == icm_dev.fifo_is_used)
		{
			if( icm_dev.fifo_highres_enabled)
				/* Use little endian mode */
				data->temperature = (int16_t)(buf[0] | buf[1] << 8);
			else data->temperature = (int8_t)buf[0];
		} 
		else 
			/* Use little endian mode */
			data->temperature = (int16_t)(buf[0] | buf[1] << 8);
          
		return;
	}

    if( TS==sensorType && true == icm_dev.fifo_is_used) 
	{		
		/* Use little endian mode */
        cur_fifo_ts = (uint16_t)(buf[0] | buf[1] << 8);
	    
        //INV_LOG(SENSOR_LOG_LEVEL, "TS reg %x %x", buf[1],buf[0]);

        if (cur_fifo_ts != icm_dev.pre_fifo_ts) {
            /* Check for possible overflow */
            if (cur_fifo_ts < icm_dev.pre_fifo_ts) {
                icm_dev.totol_sensor_ts += cur_fifo_ts + (0xFFFF - icm_dev.pre_fifo_ts);
            } else {
                icm_dev.totol_sensor_ts += (cur_fifo_ts - icm_dev.pre_fifo_ts);
            }

            icm_dev.pre_fifo_ts = cur_fifo_ts;  
            //INV_LOG(SENSOR_LOG_LEVEL, "totol %lld cur %d, pre %d",  icm_dev.totol_sensor_ts,cur_fifo_ts,icm_dev.pre_fifo_ts);
        }
        data->timeStamp  = icm_dev.totol_sensor_ts;
	}
    
	if( (ACC==sensorType)||(GYR==sensorType) ) 
	{
		/* Use little endian mode */
		data->x = (int16_t)(buf[0] | buf[1] << 8);
		data->y = (int16_t)(buf[2] | buf[3] << 8);
		data->z = (int16_t)(buf[4] | buf[5] << 8);
		data->sensType = sensorType;
	
		if( (icm_dev.fifo_highres_enabled)&&(icm_dev.fifo_is_used) )
		{
			
			high_res_rel_addr = 0 + FIFO_ACCEL_GYRO_HIGH_RES_DATA_SHIFT - cur_index;
	
			data->high_res[0] = buf[high_res_rel_addr]; 
			data->high_res[1] = buf[high_res_rel_addr+1];
			data->high_res[2] = buf[high_res_rel_addr+2];	
	
			if( ACC==sensorType )
			{
				for (i=0; i<3; i++)
					data->high_res[i] = (data->high_res[i] & 0xF0 )>>4;
			}
			else
			{
				for (i=0; i<3; i++)
					data->high_res[i] &= 0x0F;
			}
		}
	} 
}

static int inv_imu_convert_rawdata(struct accGyroDataPacket *packet)
{
    int ret = 0;
    uint32_t i = 0;
    uint8_t accEventSize = 0;
    uint8_t gyroEventSize = 0;
    uint8_t accEventSize_Discard = 0;
    uint8_t gyroEventSize_Discard = 0;
    uint64_t tick_ts = 0;
    int16_t temper  = 0;
    
    struct accGyroData *data = packet->outBuf;

    if (true == icm_dev.fifo_is_used && false == icm_dev.polling_data_en) { //fifo mode
        for (i = 0; i < icm_dev.fifoDataToRead; i += icm_dev.fifo_packet_size) {
            //INV_LOG(SENSOR_LOG_LEVEL, "Fifo head format is 0x%x", icm_dev.dataBuf[i]);
            if ((accEventSize + gyroEventSize) < MAX_RECV_PACKET*2) {
                if ((true == icm_dev.sensors[ACC].configed) ||
                    (true == icm_dev.sensors[GYR].configed)){

                    inv_imu_parse_rawdata(&(icm_dev.dataBuf[i + FIFO_TEMP_DATA_SHIFT]),FIFO_TEMP_DATA_SHIFT, TEMP, &data[accEventSize + gyroEventSize]);
                    temper = data[accEventSize+gyroEventSize].temperature;      // Copy temperature data to gyro record
                    // data[accEventSize+gyroEventSize-2].temperature = data[accEventSize+gyroEventSize-1].temperature;

                    if(icm_dev.tmst_is_used){
                        inv_imu_parse_rawdata(&(icm_dev.dataBuf[i + FIFO_TIMESTAMP_DATA_SHIFT]), FIFO_TIMESTAMP_DATA_SHIFT,TS, &data[accEventSize + gyroEventSize]);
                        // Copy TS data to gyro record
                        tick_ts = data[accEventSize + gyroEventSize].timeStamp;
                   }
                }
                
                if ((true == icm_dev.sensors[ACC].powered) &&
                    (true == icm_dev.sensors[ACC].configed)) {
                    if (icm_dev.sensors[ACC].samplesToDiscard) {
                        icm_dev.sensors[ACC].samplesToDiscard--;
                        accEventSize_Discard++;
                    } else {
                        inv_imu_parse_rawdata(&(icm_dev.dataBuf[i + FIFO_ACCEL_DATA_SHIFT]), FIFO_ACCEL_DATA_SHIFT,ACC, &data[accEventSize + gyroEventSize]);
                        accEventSize++;
                    }
                }
                if ((true == icm_dev.sensors[GYR].powered) &&
                    (true == icm_dev.sensors[GYR].configed)) {
                    if (icm_dev.sensors[GYR].samplesToDiscard) {
                        icm_dev.sensors[GYR].samplesToDiscard--;
                        gyroEventSize_Discard++;
                    } else {
                        inv_imu_parse_rawdata(&(icm_dev.dataBuf[i + FIFO_GYRO_DATA_SHIFT]),FIFO_GYRO_DATA_SHIFT,GYR, &data[accEventSize + gyroEventSize]);
                        data[accEventSize + gyroEventSize].timeStamp = tick_ts;
                        data[accEventSize + gyroEventSize].temperature = temper;
                        gyroEventSize++;
                    }
                }
          
            } else {
                INV_LOG(INV_LOG_LEVEL_ERROR, "outBuf full, accEventSize = %d, gyroEventSize = %d", accEventSize, gyroEventSize);
                ret = FIFO_DATA_FULL_ERROR;
            }
        }
    } else { //dri mode or polling mode 
        if ((true == icm_dev.sensors[ACC].configed) &&
            (true == icm_dev.sensors[ACC].powered)) {
            if (icm_dev.sensors[ACC].samplesToDiscard) {
                icm_dev.sensors[ACC].samplesToDiscard--;
                accEventSize_Discard++;
            } else {
                inv_imu_parse_rawdata(&(icm_dev.dataBuf[DRI_ACCEL_DATA_SHIFT]),DRI_ACCEL_DATA_SHIFT, ACC, &data[accEventSize + gyroEventSize]);
                data[accEventSize + gyroEventSize].timeStamp = tick_ts;
                accEventSize++;
            }
        }
        if ((true == icm_dev.sensors[GYR].configed) &&
            (true == icm_dev.sensors[GYR].powered)) {
            if (icm_dev.sensors[GYR].samplesToDiscard) {
                icm_dev.sensors[GYR].samplesToDiscard--;
                gyroEventSize_Discard++;
            } else {
                inv_imu_parse_rawdata(&(icm_dev.dataBuf[DRI_GYRO_DATA_SHIFT]), DRI_GYRO_DATA_SHIFT,GYR, &data[accEventSize + gyroEventSize]);
                data[accEventSize + gyroEventSize].timeStamp = tick_ts;
                gyroEventSize++;
            }
        }
        if ((true == icm_dev.sensors[ACC].configed) ||
            (true == icm_dev.sensors[GYR].configed))
        {
            inv_imu_parse_rawdata(&(icm_dev.dataBuf[0]),0, TEMP, &data[accEventSize + gyroEventSize-1]);

            // Copy temperature data to ACC record
            data[accEventSize+gyroEventSize-2].temperature = data[accEventSize+gyroEventSize-1].temperature;
        }
    }

    packet->accOutSize = accEventSize;
    packet->gyroOutSize = gyroEventSize;
    packet->temperature = icm_dev.chip_temper;
    packet->timeStamp = 0;
    
    return ret;
}

int inv_imu_read_rawdata(void)
{
    int ret = 0;
    
    ret += inv_imu_read_reg(TEMP_DATA1, icm_dev.dri_packet_size, icm_dev.dataBuf);
    
    return ret;
}


int inv_imu_polling_rawdata(struct accGyroDataPacket *dataPacket)
{
    int ret = 0;

	icm_dev.dri_packet_size = DRI_14BYTES_PACKET_SIZE;
	//icm_dev.polling_data_en = true;  // enable polling flag when polling data
	
	if (dataPacket == NULL) {
        INV_LOG(INV_LOG_LEVEL_ERROR, "DATAPACKET NULL POINTER!!");
        EXIT(-1);
    }

	ret += inv_imu_read_rawdata();

	if (ret != 0) {
        INV_LOG(INV_LOG_LEVEL_ERROR, "Read Raw Data Error %d", ret);
        return DRDY_DATA_READ_ERROR;
        }
    ret += inv_imu_convert_rawdata(dataPacket);

	if (ret != 0) {
        INV_LOG(INV_LOG_LEVEL_ERROR, "Convert Raw Data Error %d", ret);
        return DRDY_DATA_CONVERT_ERROR;
        }
	//icm_dev.polling_data_en = false;  // disable polling flag when finish polling data
	return ret;
}


int inv_imu_get_rawdata_interrupt(struct accGyroDataPacket *dataPacket)
{
    int_status_t int_status_reg;
    int_status_drdy_t int_status_drdy;
    int ret = 0;
	bool pre_polling_mode = icm_dev.polling_data_en;

	if (dataPacket == NULL) {
        INV_LOG(INV_LOG_LEVEL_ERROR, "DATAPACKET NULL POINTER!!");
        EXIT(-1);
    }
    /* Ensure data ready status bit is set */
	if((ret |= inv_imu_read_reg(INT_STATUS, 1, (uint8_t*)&int_status_reg)))
		return INT_STATUS_READ_ERROR;

	if((ret |= inv_imu_read_reg(INT_STATUS_DRDY, 1, (uint8_t*)&int_status_drdy)))
		return ret;
		
    //INV_LOG(SENSOR_LOG_LEVEL, "INT_STATUS 0x%x, int_status_drdy 0x%x", int_status,int_status_drdy);

    /* Read data from data register according to DRI (data ready interrupt) */
		//force polling sensor data once because polling request
	if(icm_dev.polling_data_en){
		ret += inv_imu_polling_rawdata(dataPacket);
        if (ret != 0) {
            INV_LOG(INV_LOG_LEVEL_ERROR, "force polling_rawdata Error %d", ret);
            return DRDY_DATA_CONVERT_ERROR;
        }
	}
    else if ((INT_STATUS_DRDY_DATA_RDY_INT_GENERATED==int_status_drdy.data_rdy_int) && (false == icm_dev.fifo_is_used)) {
        INV_LOG(SENSOR_LOG_LEVEL, "DRDY INT Detected");
        if ((false == icm_dev.sensors[ACC].configed) &&
            (false == icm_dev.sensors[GYR].configed)) {
            INV_LOG(INV_LOG_LEVEL_ERROR, "Unexpected DRDY INTR fired");
            return DRDY_UNEXPECT_INT_ERROR;
        }

        ret += inv_imu_polling_rawdata(dataPacket);
        if (ret != 0) {
            INV_LOG(INV_LOG_LEVEL_ERROR, "inv_imu_polling_rawdata Error %d", ret);
            return DRDY_DATA_CONVERT_ERROR;
        }
    }

    /* Read data from FIFO */
    if (INT_STATUS_FIFO_FULL_INT_GENERATED==int_status_reg.fifo_full_int) {
        INV_LOG(INV_LOG_LEVEL_ERROR, "FIFO Overflow!!!");
        // reset fifo
        inv_imu_reset_fifo();
        return FIFO_OVERFLOW_ERROR;
    } else if ( INT_STATUS_FIFO_THS_INT_GENERATED==int_status_reg.fifo_ths_int) {
		
		icm_dev.polling_data_en = false;  // disable polling flag when operate fifo data
		
        //INV_LOG(SENSOR_LOG_LEVEL, "WM INT Detected");
        if ((false == icm_dev.sensors[ACC].configed) &&
            (false == icm_dev.sensors[GYR].configed)) {
            INV_LOG(SENSOR_LOG_LEVEL, "Unexpected FIFO WM INTR fired");
            // reset fifo
            inv_imu_reset_fifo();
            return FIFO_UNEXPECT_WM_ERROR;
        }
        ret += inv_imu_read_fifo();
        if (ret != 0) {
            INV_LOG(INV_LOG_LEVEL_ERROR, "Fifo Data Read Error %d", ret);
            return FIFO_DATA_READ_ERROR;
        }
        ret += inv_imu_convert_rawdata(dataPacket);
		icm_dev.polling_data_en = pre_polling_mode;  //recover pre polling status
		
        if (ret != 0) {
            INV_LOG(INV_LOG_LEVEL_ERROR, "Fifo Data Convert Error %d", ret);
            return FIFO_DATA_CONVERT_ERROR;
        }
    }
    /* else: FIFO threshold was not reached and FIFO was not full */
    
    return ret;
}

void inv_apply_mounting_matrix(int32_t raw[3])
{
	int32_t data[3];
	int i=0;

	for (i=0; i<3; i++)
		data[icm_dev.cvt.axis[i]] = icm_dev.cvt.sign[i] * raw[i];

	for (i=0; i<3; i++)
		raw[i]= data[i];
}

int inv_data_handler(AccDataPacket *accDatabuff,GyroDataPacket *gyroDatabuff,
                     chip_temperature *chip_temper,bool polling)
{
    int ret=0;
	int32_t data[3];
    int acc_index = 0;
    int gyro_index = 0;
	int i;
    struct accGyroDataPacket *datapacket = (struct accGyroDataPacket *)calloc(1, sizeof(struct accGyroDataPacket));
    if (NULL == datapacket)
    {
        INV_LOG(INV_LOG_LEVEL_ERROR, "malloc for datapacket failed");
        ret = INV_ERROR_MEM;
        goto err;
    }

	AccDataPacket *AccDatabuff = accDatabuff;
	GyroDataPacket *GyroDatabuff = gyroDatabuff;
	icm_dev.polling_data_en = polling;
	
    /* Raw A+G */
    if (true == icm_dev.sensors[ACC].configed || icm_dev.sensors[GYR].configed) 
    {
        if(0 != inv_imu_get_rawdata_interrupt(datapacket))
        {
            ret = INV_ERROR;
            goto err;
        }

    #if !SENSOR_LOG_TS_ONLY
        INV_LOG(SENSOR_LOG_LEVEL, "Get Sensor Data ACC %d GYR %d ",
            datapacket->accOutSize, datapacket->gyroOutSize);
    #endif
        AccDatabuff->accDataSize = datapacket->accOutSize;
        GyroDatabuff->gyroDataSize = datapacket->gyroOutSize;
    
        for (i = 0; i < datapacket->accOutSize + datapacket->gyroOutSize; i++) 
        {
	        if(IS_HIGH_RES_MODE)
	        {
				data[0] = ((int32_t)datapacket->outBuf[i].x <<4) | datapacket->outBuf[i].high_res[0];
				data[1] = ((int32_t)datapacket->outBuf[i].y <<4) | datapacket->outBuf[i].high_res[1];
				data[2] = ((int32_t)datapacket->outBuf[i].z <<4) | datapacket->outBuf[i].high_res[2];
			}
			else
			{
				data[0] = (int32_t)datapacket->outBuf[i].x;
				data[1] = (int32_t)datapacket->outBuf[i].y;
				data[2] = (int32_t)datapacket->outBuf[i].z;
			}

	#if DATA_FORMAT_DPS_G  // Output data in dps/g/degree format.
			inv_apply_mounting_matrix(data);

			/* convert to physical unit: Celsius degree */
			if (FIFO_WM_MODE_EN && (!IS_HIGH_RES_MODE))  // Normal FIFO mode
				*chip_temper = ((float)datapacket->outBuf[i].temperature * TEMP_SENSITIVITY_1_BYTE) + ROOM_TEMP_OFFSET;
			else  // FIFO High Resolution mode and DRI mode.
				*chip_temper = ((float)datapacket->outBuf[i].temperature * TEMP_SENSITIVITY_2_BYTE) + ROOM_TEMP_OFFSET;

			if( ACC==datapacket->outBuf[i].sensType ) 
			{
			    AccDatabuff->databuff[acc_index].x = (float)data[0] * ACC_RESOLUTION_HIRES(icm_dev.acc_cfg0.accel_ui_fs_sel,IS_HIGH_RES_MODE);
                AccDatabuff->databuff[acc_index].y = (float)data[1] * ACC_RESOLUTION_HIRES(icm_dev.acc_cfg0.accel_ui_fs_sel,IS_HIGH_RES_MODE);
                AccDatabuff->databuff[acc_index].z = (float)data[2] * ACC_RESOLUTION_HIRES(icm_dev.acc_cfg0.accel_ui_fs_sel,IS_HIGH_RES_MODE);
                AccDatabuff->databuff[acc_index].timeStamp =((uint64_t) datapacket->outBuf[i].timeStamp)*16;

                #if !SENSOR_LOG_TS_ONLY
                INV_LOG(SENSOR_LOG_LEVEL, "ACC TS  %lld C %.1f %.1f %.1f %.1f", AccDatabuff->databuff[acc_index].timeStamp, *chip_temper, AccDatabuff->databuff[acc_index].x, AccDatabuff->databuff[acc_index].y, AccDatabuff->databuff[acc_index].z );           
                #endif
                acc_index ++;
            }
			else if(GYR==datapacket->outBuf[i].sensType) 
			{
		
				GyroDatabuff->databuff[gyro_index].x = (float)data[0] * GYRO_RESOLUTION_HIRES(icm_dev.gyro_cfg0.gyro_ui_fs_sel,IS_HIGH_RES_MODE);
                GyroDatabuff->databuff[gyro_index].y = (float)data[1] * GYRO_RESOLUTION_HIRES(icm_dev.gyro_cfg0.gyro_ui_fs_sel,IS_HIGH_RES_MODE);
                GyroDatabuff->databuff[gyro_index].z = (float)data[2] * GYRO_RESOLUTION_HIRES(icm_dev.gyro_cfg0.gyro_ui_fs_sel,IS_HIGH_RES_MODE);
                GyroDatabuff->databuff[gyro_index].timeStamp = ((uint64_t) datapacket->outBuf[i].timeStamp)*16;

                #if !SENSOR_LOG_TS_ONLY
				INV_LOG(SENSOR_LOG_LEVEL, "GYR  TS %lld C %.1f %.1f %.1f %.1f", GyroDatabuff->databuff[gyro_index].timeStamp, *chip_temper, GyroDatabuff->databuff[gyro_index].x, GyroDatabuff->databuff[gyro_index].y, GyroDatabuff->databuff[gyro_index].z );
                #endif
                gyro_index ++;
            }
						
	#else  // Output data in LSB format.
            if ( ACC==datapacket->outBuf[i].sensType ){
                
                AccDatabuff->databuff[acc_index].x = data[0] ;
                AccDatabuff->databuff[acc_index].y = data[1] ;
                AccDatabuff->databuff[acc_index].z = data[2] ;
                
                AccDatabuff->databuff[acc_index].timeStamp = (uint64_t) datapacket->outBuf[i].timeStamp *16;
                *chip_temper  = datapacket->outBuf[i].temperature;
                acc_index ++;

                #if !SENSOR_LOG_TS_ONLY
                INV_LOG(SENSOR_LOG_LEVEL, "ACC data TS %lld C %d %d  %d %d", datapacket->outBuf[i].timeStamp, datapacket->outBuf[i].temperature, data[0], data[1], data[2]);
                #endif
            }
            else if ( GYR==datapacket->outBuf[i].sensType ){
                GyroDatabuff->databuff[gyro_index].x = data[0];
                GyroDatabuff->databuff[gyro_index].y = data[1];
                GyroDatabuff->databuff[gyro_index].z = data[2];
                GyroDatabuff->databuff[gyro_index].timeStamp =(uint64_t)datapacket->outBuf[i].timeStamp *16;
                *chip_temper = datapacket->outBuf[i].temperature;

                gyro_index ++;
                #if !SENSOR_LOG_TS_ONLY
                INV_LOG(SENSOR_LOG_LEVEL, "GYR data TS %lld C %d  %d  %d %d", datapacket->outBuf[i].timeStamp, datapacket->outBuf[i].temperature, data[0], data[1], data[2]);
                #endif
                }
    #endif
        }

    }
    return ret;
err:
    free(datapacket);
    return ret;
}

int inv_imu_run_selftest(uint8_t acc_control,uint8_t gyro_control, struct inv_imu_selftest_output * st_output)
{
	uint8_t st_done;
    int_status_t reg_int_status;
	int result = 0;
    int count =0;

	result |= inv_imu_switch_on_mclk();

	result |= inv_imu_start_dmp_selftest();
	result |= inv_imu_configure_selftest_parameters();
	/* RC is kept on before this function in order to guarantee proper access to MCLK register without having to rewrite SCLK */
	result |= inv_imu_control_selftest(acc_control,gyro_control);

	/* check st_status1/2 (active polling) */
	st_done = 0;

    if( (SELFTEST_ACCEL_ST_EN_DIS==acc_control) && (SELFTEST_GYRO_ST_EN_DIS==gyro_control) ) {
		/* Nothing else required if self-test is not being run */ 
        INV_LOG(SENSOR_LOG_LEVEL, "no selftest item enable and need to run");
		return 0;
	}
    
	do {
        inv_delay_ms(1);
	    result |= inv_imu_read_reg(INT_STATUS, 1, (uint8_t*)&reg_int_status);
	    st_done = reg_int_status.st_int;
	   	count ++;
        
        if(count > 5000)
             INV_LOG(SENSOR_LOG_LEVEL, "selftest timeout no result return %d",count);
	} while ((!st_done) && (count < 5000) );

	/* report self-test status */
	result |= inv_imu_process_selftest_end(st_output);
	
	/* Restore idle bit */
	result |= inv_imu_switch_off_mclk();

    /* reset device and reinit sensor */
    result |= inv_imu_init_config();
	return result;
}

int inv_imu_start_dmp_selftest(void)
{
	int status = 0;
	mclk_rdy_t mclk_rdy;
    otp_config_t otp_config;
    otp_ctrl7_t otp_ctrl7;

	/* Disables Gyro/Accel sensors */	
    status |= inv_imu_read_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);
    
    icm_dev.pwr_mgmt.gyro_mode = PWR_MGMT_0_GYRO_MODE_OFF;
    icm_dev.pwr_mgmt.accel_mode = PWR_MGMT_0_ACCEL_MODE_OFF;

	status |= inv_imu_write_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);

    icm_dev.sensors[GYR].powered = false;
    icm_dev.sensors[ACC].powered = false;
    
	// Reset SRAM to 0's
	status |= inv_imu_reset_dmp();
	if(status)
		return status;

	// APEX algorithms will restart from scratch after self-test
	icm_dev.dmp_is_on = false;

	// Trigger OTP load for ST data
	status |= inv_imu_read_reg(OTP_CONFIG, 1, (uint8_t*)&otp_config);	
	otp_config.otp_copy_mode = OTP_COPY_EN_DATA;    
	status |= inv_imu_write_reg(OTP_CONFIG, 1, (uint8_t*)&otp_config);

	status |= inv_imu_read_reg(OTP_CTRL7, 1, (uint8_t*)&otp_ctrl7);
	otp_ctrl7.otp_pwr_down = OTP_CTRL7_PWR_UP;
	status |= inv_imu_write_reg(OTP_CTRL7, 1,  (uint8_t*)&otp_ctrl7);
	
	inv_delay_us(100);

	/* Host should disable INT function first before kicking off OTP copy operation */
    status |= inv_imu_read_reg(OTP_CTRL7, 1, (uint8_t*)&otp_ctrl7);
	otp_ctrl7.otp_reload= OTP_CTRL7_RELOAD_EN;
    status |= inv_imu_write_reg(OTP_CTRL7, 1,  (uint8_t*)&otp_ctrl7);

	inv_delay_us(20);

	// Sanity check to reload autocleared and otp_done is raised
    status |= inv_imu_read_reg(OTP_CTRL7, 1, (uint8_t*)&otp_ctrl7);
    
	status |= inv_imu_read_reg(MCLK_RDY, 1, (uint8_t*)&mclk_rdy);
	if((OTP_DONE_NO==mclk_rdy.otp_done) || (otp_ctrl7.otp_reload != OTP_CTRL7_RELOAD_DIS)){
    INV_LOG(SENSOR_LOG_LEVEL, "misc_otp_value %x , otp_reload %x  not right !!!!\n",mclk_rdy,otp_ctrl7.otp_reload);    
        return INV_ERROR_UNEXPECTED;
    }
	
    icm_dev.int_src0.st_int1_en = 1;
    status |= inv_imu_write_reg(INT_SOURCE0,1,(uint8_t*)&icm_dev.int_src0);
    INV_LOG(SENSOR_LOG_LEVEL, "Enable ST INT1 0x%x", icm_dev.int_src0);

	return status;
}

int inv_imu_configure_selftest_parameters(void)
{
	int status = 0;
    selftest_t reg_selftest;
    st_config_t reg_st_config;
    uint8_t st_num_samples = ST_CONFIG_ST_NUMBER_SAMPLE_16;
	uint8_t accel_limit = ST_CONFIG_ACCEL_ST_LIM_50;
	uint8_t gyro_limit = ST_CONFIG_GYRO_ST_LIM_50;

	inv_imu_switch_on_mclk();		

	/* Self-test configuration cannot be updated if it already running */
	status |= inv_imu_read_reg(SELFTEST_MREG, 1, (uint8_t*)&reg_selftest);
    /* make sure there is no selftest running configuration */
	if( reg_selftest.accel_st_en ==1 || reg_selftest.gyro_st_en ==1)
		return INV_ERROR_UNEXPECTED;

	status |= inv_imu_read_reg(ST_CONFIG_MREG, 1, (uint8_t*)&reg_st_config);
    reg_st_config.st_num_sample = st_num_samples;
    reg_st_config.accel_st_lim = accel_limit;
    reg_st_config.gyro_st_lim = gyro_limit;
	status |= inv_imu_write_reg(ST_CONFIG_MREG, 1, (uint8_t*)&reg_st_config);

	status |= inv_imu_switch_off_mclk();
	
	return status;
}

int inv_imu_control_selftest(uint8_t acc_control,uint8_t gyro_control)
{
	/* This function can be used to either enable or abort self-test */
    selftest_t reg_selftest;
	int status = 0;

	status |= inv_imu_read_reg(SELFTEST_MREG, 1, (uint8_t*)&reg_selftest);
	reg_selftest.accel_st_en = acc_control;
    reg_selftest.gyro_st_en  = gyro_control;
    
	/* Both accel and gyro MUST be enabled simulaneously for DMP to take it into account properly */
	status |= inv_imu_write_reg(SELFTEST_MREG, 1, (uint8_t*)&reg_selftest);
	
	return status;
}

int inv_imu_process_selftest_end(inv_imu_selftest_output_t *st_output)
{
	
	int status = 0;
    //uint8_t data;
    st_status1_t reg_st_status1;
    st_status2_t reg_st_status2;
    selftest_t reg_selftest;
    otp_ctrl7_t otp_ctrl7;
    
    inv_imu_selftest_output_t *st_result = st_output;
        
	status |= inv_imu_read_reg(ST_STATUS1_MREG, 1, (uint8_t*)&reg_st_status1);
    status |= inv_imu_read_reg(ST_STATUS2_MREG, 1, (uint8_t*)&reg_st_status2);

    INV_LOG(SENSOR_LOG_LEVEL, "ST_STATUS1  0x%x  ST_STATUS2 0x%x ", reg_st_status1,reg_st_status2);
	if(0 == status) {
		st_result->accel_status = reg_st_status1.accel_st_pass;	
		st_result->gyro_status =  reg_st_status2.gyro_st_pass | (reg_st_status2.st_incomplete << 1);
    }

    if (st_result->accel_status  == 0x1)
        INV_LOG(SENSOR_LOG_LEVEL,  "accel Selftest PASS\n");
    else
        INV_LOG(SENSOR_LOG_LEVEL,  "accel Selftest fail\n");
    
    if (st_result->gyro_status  == 0x1){
        INV_LOG(SENSOR_LOG_LEVEL,  "gyro Selftest PASS\n");
    }else{
        INV_LOG(SENSOR_LOG_LEVEL,  "gyro Selftest fail\n");
    }   

	/* Turn off self-test */
	status |= inv_imu_read_reg(SELFTEST_MREG, 1, (uint8_t*)&reg_selftest);
    reg_selftest.accel_st_en = 0;
    reg_selftest.gyro_st_en = 0;
	status |= inv_imu_write_reg(SELFTEST_MREG, 1, (uint8_t*)&reg_selftest);

	/* Re-sync all clocking scheme with a dummy write 
	data = 0;
	status |= inv_imu_write_reg(WHO_AM_I, 1, &data);*/

	/* Turn off OTP macro */

    status |= inv_imu_read_reg(OTP_CTRL7, 1, (uint8_t*)&otp_ctrl7);
	otp_ctrl7.otp_pwr_down = OTP_CTRL7_PWR_DOWN;
	status |= inv_imu_write_reg(OTP_CTRL7, 1,  (uint8_t*)&otp_ctrl7);
	
	inv_delay_ms(20);//100-1000u

	return status;
}

/*  DMP Power save mode */
static int inv_imu_dmp_powersave(bool powersave)
{   
   int status = 0;
	apex_config0_t reg_apex_config0;
    wom_config_t wom_config_reg;
    /* APEX_CONFIG0 */
	status |= inv_imu_read_reg(APEX_CONFIG0, 1, (uint8_t*)&reg_apex_config0);
	
	if(true == powersave){
	    reg_apex_config0.dmp_power_save_en = APEX_CONFIG0_DMP_POWER_SAVE_EN;
    } else {
	    reg_apex_config0.dmp_power_save_en = APEX_CONFIG0_DMP_POWER_SAVE_DIS;
    }

    status |= inv_imu_write_reg(APEX_CONFIG0, 1, (uint8_t*)&reg_apex_config0);
 
    if(true == powersave){;
          
        /* Enable WOM */
        status |= inv_imu_read_reg(WOM_CONFIG, 1, (uint8_t*)&wom_config_reg);
        wom_config_reg.wom_en = WOM_CONFIG_WOM_EN_ENABLE;
        status |= inv_imu_write_reg(WOM_CONFIG, 1, (uint8_t*)&wom_config_reg);
    } 
	INV_LOG(SENSOR_LOG_LEVEL, "DMP_powersave apex_config0 %x, wom_cfg %x \n", reg_apex_config0,wom_config_reg);

    return status;
}


static int resume_dmp(void)
{
	int status = 0;
	apex_config0_t reg_apex_config0;
	uint16_t count=0;

	status |= inv_imu_read_reg(APEX_CONFIG0, 1, (uint8_t*)&reg_apex_config0);
    reg_apex_config0.dmp_init_en =  APEX_CONFIG0_DMP_INIT_EN;
	status |= inv_imu_write_reg(APEX_CONFIG0, 1, (uint8_t*)&reg_apex_config0);

    INV_LOG(SENSOR_LOG_LEVEL, "resume_dmp apex_config0 %x\n", reg_apex_config0);

    inv_delay_ms(50);   //test 50ms first
	/* wait to make sure dmp_init_en = 0 */
	do {
        inv_delay_us(100);
		inv_imu_read_reg(APEX_CONFIG0, 1, (uint8_t*)&reg_apex_config0);

		if (reg_apex_config0.dmp_init_en== 0)
			break;
	    count++;
	} while (count < 500);

	return status;
}

static int inv_imu_start_dmp(void)
{
	int status = 0;
	
	// On first enabling of DMP, reset internal state
	if(!icm_dev.dmp_is_on) {
		// Reset SRAM to 0's
		status |= inv_imu_reset_dmp();
		if(status)
			return status;
		icm_dev.dmp_is_on = true;
	}
	
	// Initialize DMP
	status |= resume_dmp();
	
	return status;
}

int inv_imu_reset_dmp(void)
{
	const int ref_timeout = 5000; /*50 ms*/
	int status = 0;
	int timeout = ref_timeout;
	apex_config0_t  reg_apex_config0;
			
	status |= inv_imu_switch_on_mclk();

	// Reset DMP internal memories
	status |= inv_imu_read_reg(APEX_CONFIG0, 1, (uint8_t*)&reg_apex_config0);
    reg_apex_config0.dmp_mem_reset_en = APEX_CONFIG0_DMP_MEM_RESET_EN;
	status |= inv_imu_write_reg(APEX_CONFIG0, 1, (uint8_t*)&reg_apex_config0);

    INV_LOG(SENSOR_LOG_LEVEL, "inv_imu_reset_dmp w APEX_CONFIG0  %x \n", reg_apex_config0);
    
	inv_delay_ms(1);

	// Make sure reset procedure has finished by reading back mem_reset_en bit
	do {
		inv_delay_us(10);
		status |= inv_imu_read_reg(APEX_CONFIG0, 1, (uint8_t*)&reg_apex_config0);
	} while ((reg_apex_config0.dmp_mem_reset_en  != APEX_CONFIG0_DMP_MEM_RESET_DIS) && timeout-- && !status);

	status |= inv_imu_switch_off_mclk();

	if (timeout <= 0){
        INV_LOG(SENSOR_LOG_LEVEL, "time out reg %x \n", reg_apex_config0);
		return INV_ERROR_TIMEOUT;
       }
    
	return status;
}

int inv_imu_configure_wom(const uint8_t wom_x_th, const uint8_t wom_y_th, const uint8_t wom_z_th,
                               WOM_CONFIG_WOM_INT_MODE_t wom_int, WOM_CONFIG_WOM_INT_DUR_t wom_dur)
{
	int status = 0;
	uint8_t data[3];
    wom_config_t wom_config_reg;
	
	data[0] = wom_x_th; // Set X threshold
	data[1] = wom_y_th; // Set Y threshold
	data[2] = wom_z_th; // Set Z threshold
	status |= inv_imu_write_reg(ACCEL_WOM_X_THR_MREG, sizeof(data), &data[0]);
	
	// Compare current sample with the previous sample and WOM from the 3 axis are ORed or ANDed to produce WOM signal.
	status |= inv_imu_read_reg(WOM_CONFIG, 1, (uint8_t*)&wom_config_reg);
	wom_config_reg.wom_int_mode = wom_int;
    wom_config_reg.wom_mode =  WOM_CONFIG_WOM_MODE_CMP_PREV;   
	
	// Configure the number of overthreshold event to wait before producing the WOM signal.
	
	wom_config_reg.wom_int_dur = wom_dur;
	status |= inv_imu_write_reg(WOM_CONFIG, 1, (uint8_t*)&wom_config_reg);

	return status;
}

int inv_imu_enable_wom_register(void)
{
	int status = 0;
    wom_config_t wom_config_reg;
    int_source1_t int_source1_reg;
		
	/* Enable WOM */
	status |= inv_imu_read_reg(WOM_CONFIG, 1, (uint8_t*)&wom_config_reg);
    wom_config_reg.wom_en = WOM_CONFIG_WOM_EN_ENABLE;
    status |= inv_imu_write_reg(WOM_CONFIG, 1, (uint8_t*)&wom_config_reg);

	/* enable  wom int1 */
	status |= inv_imu_read_reg(INT_SOURCE1, 1, (uint8_t*)&int_source1_reg);
    int_source1_reg.wom_x_int1_en = 1;
    int_source1_reg.wom_y_int1_en = 1;
    int_source1_reg.wom_z_int1_en = 1;
    status |= inv_imu_write_reg(INT_SOURCE1, 1, (uint8_t*)&int_source1_reg);

	return status;
}

int inv_imu_disable_wom_register(bool wom_disable)
{
	int status = 0;
    wom_config_t wom_config_reg;
    int_source1_t int_source1_reg;
		
	/* disable WOM */
    if(wom_disable){
	    status |= inv_imu_read_reg(WOM_CONFIG, 1, (uint8_t*)&wom_config_reg);
        wom_config_reg.wom_en = WOM_CONFIG_WOM_EN_DISABLE;
        status |= inv_imu_write_reg(WOM_CONFIG, 1, (uint8_t*)&wom_config_reg);
    }

	/* disable wom int1 */
	status |= inv_imu_read_reg(INT_SOURCE1, 1, (uint8_t*)&int_source1_reg);
    int_source1_reg.wom_x_int1_en = 0;
    int_source1_reg.wom_y_int1_en = 0;
    int_source1_reg.wom_z_int1_en = 0;
    status |= inv_imu_write_reg(INT_SOURCE1, 1, (uint8_t*)&int_source1_reg);

	return status;
}

int inv_imu_wom_enable(uint8_t wom_threshold_x,uint8_t wom_threshold_y,uint8_t wom_threshold_z,uint8_t duration)
{
    int ret = 0;
    uint8_t regValue = 0;
    int odr_index = 0;
    uint32_t sampleRate = 0;

    // if acc is not in streaming mode enable acc
    if (!icm_dev.sensors[ACC].configed) {
        if(icm_dev.sensors[GYR].configed)
            icm_dev.sensors[ACC].hwRate = icm_dev.sensors[GYR].hwRate;
        else
            icm_dev.sensors[ACC].hwRate = icm_dev.min_apex_odr;
      
        odr_index = inv_imu_cal_odr(&icm_dev.sensors[ACC].hwRate, &sampleRate);
        regValue = IMU_ODR_MAPPING[odr_index];
        INV_LOG(SENSOR_LOG_LEVEL, "set acc Odr Reg value 0x%x", regValue);
         /* update new odr */

        icm_dev.acc_cfg0.accel_odr = regValue;
        ret += inv_imu_write_reg(ACCEL_CONFIG0,1, (uint8_t*)&icm_dev.acc_cfg0);
        INV_LOG(SENSOR_LOG_LEVEL, "write ACCEL_CONFIG0  0x%x", icm_dev.acc_cfg0); 

    
        inv_delay_us(200);
        INV_LOG(SENSOR_LOG_LEVEL, "wom enable, acc on");
    }

    //config acc power mode
    if((!icm_dev.sensors[ACC].configed) && (!icm_dev.sensors[GYR].configed))
        ret += inv_imu_enable_accel_low_power_mode(1024000000 / icm_dev.sensors[ACC].hwRate);
    else
        ret += inv_imu_enable_accel_low_noise_mode(1024000000 / icm_dev.sensors[ACC].hwRate);
    
    /* set X,Y,Z threshold */   
    ret += inv_imu_configure_wom(wom_threshold_x,wom_threshold_y,wom_threshold_z,
           WOM_CONFIG_WOM_INT_MODE_ORED,(WOM_CONFIG_WOM_INT_DUR_t)duration);
    
    ret += inv_imu_enable_wom_register();
    if (ret != 0)
        return WOM_ENABLE_ERROR;

    icm_dev.sensors[WOM].powered = true;

    #if SENSOR_REG_DUMP
    inv_imu_dumpRegs();
    #endif
    
    return ret;
}

int inv_imu_wom_disable()
{
    int ret = 0;

    INV_LOG(SENSOR_LOG_LEVEL, "%s", __func__);
    
    if(icm_dev.sensors[PEDO].powered == false) 
        ret |= inv_imu_disable_wom_register(true);         
    else   
        ret |= inv_imu_disable_wom_register(false);

    if (false == icm_dev.sensors[ACC].powered && 
        false == icm_dev.sensors[ACC].configed && 
        false == icm_dev.sensors[FF].powered ) {

        icm_dev.pwr_mgmt.accel_mode = PWR_MGMT_0_ACCEL_MODE_OFF;
        ret |= inv_imu_write_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);
        inv_delay_us(200); //spec: 200us
  
        INV_LOG(SENSOR_LOG_LEVEL, "wom disable, acc off pwr: 0x%x", icm_dev.pwr_mgmt);
    }
    
    icm_dev.sensors[WOM].powered = false;
    
    return ret;
}

int inv_imu_wom_get_event(bool *wom_detect)
{
    int_status2_t int_status2;
	int rc = 0;
	int wom_flag = 0;

    if (wom_detect == NULL) {
        INV_LOG(INV_LOG_LEVEL_ERROR, "EVENT NULL POINTER!!");
        EXIT(-1);
    }
	/*
	 *  Read WOM interrupt status
	 */
	rc = inv_imu_read_reg(INT_STATUS2, 1, (uint8_t*)&int_status2);
	if (rc != INV_ERROR_SUCCESS)
		return -1;
    
	if ( (INT_STATUS2_WOM_X_INT_GENERATED==int_status2.wom_x_int) ||
	(INT_STATUS2_WOM_Y_INT_GENERATED==int_status2.wom_y_int) ||
	(INT_STATUS2_WOM_Z_INT_GENERATED==int_status2.wom_z_int) ) {
		if(INT_STATUS2_WOM_Z_INT_GENERATED==int_status2.wom_z_int) wom_flag = 0x01;
		if(INT_STATUS2_WOM_Y_INT_GENERATED==int_status2.wom_y_int) wom_flag |= 0x02;
		if(INT_STATUS2_WOM_X_INT_GENERATED==int_status2.wom_x_int) wom_flag |= 0x04;

		INV_LOG(SENSOR_LOG_LEVEL, "WoM interrupt at (X, Y, Z): %d, %d, %d", 
				int_status2.wom_x_int,int_status2.wom_y_int, int_status2.wom_z_int );

        *wom_detect = true;       
	}

    return wom_flag;
}

int inv_imu_enable_pedo_register(void)
{
	int status = 0;
    apex_config1_t reg_apex_config1;
    int_source6_t reg_int_source6_t;

    /* enable FF int1 */
	status |= inv_imu_read_reg(INT_SOURCE6_MREG, 1, (uint8_t*)&reg_int_source6_t);
    reg_int_source6_t.step_cnt_ovfl_int1_en = EN;
    reg_int_source6_t.step_det_int1_en = EN;
    status |= inv_imu_write_reg(INT_SOURCE6_MREG, 1, (uint8_t*)&reg_int_source6_t);

    inv_delay_ms(50); 

    /* Enable PEDO */
    status |= inv_imu_read_reg(APEX_CONFIG1, 1, (uint8_t*)&reg_apex_config1);
    reg_apex_config1.ped_en = APEX_CONFIG1_PEDO_EN_EN;
    status |= inv_imu_write_reg(APEX_CONFIG1, 1, (uint8_t*)&reg_apex_config1);

	return status;
}

int inv_imu_disable_pedo_register(void)
{
	int status = 0;
    apex_config1_t reg_apex_config1;
    int_source6_t reg_int_source6_t;
		
	 /* disable PEDO int1 */
	status |= inv_imu_read_reg(INT_SOURCE6_MREG, 1, (uint8_t*)&reg_int_source6_t);
    reg_int_source6_t.step_cnt_ovfl_int1_en = DIS;
    reg_int_source6_t.step_det_int1_en = DIS;
    status |= inv_imu_write_reg(INT_SOURCE6_MREG, 1, (uint8_t*)&reg_int_source6_t);

    /* disable PEDO */
    status |= inv_imu_read_reg(APEX_CONFIG1, 1, (uint8_t*)&reg_apex_config1);
    reg_apex_config1.ped_en = APEX_CONFIG1_PEDO_EN_EN;
    status |= inv_imu_write_reg(APEX_CONFIG1, 1, (uint8_t*)&reg_apex_config1);
	return status;
}


int inv_imu_enable_ff_register(void)
{
	int status = 0;
    apex_config1_t reg_apex_config1;
    int_source6_t reg_int_source6_t;

    /* enable FF int1 */
	status |= inv_imu_read_reg(INT_SOURCE6_MREG, 1, (uint8_t*)&reg_int_source6_t);
    reg_int_source6_t.ff_int1_en = EN;
   
    status |= inv_imu_write_reg(INT_SOURCE6_MREG, 1, (uint8_t*)&reg_int_source6_t);

    inv_delay_ms(50); 

    /* Enable FF */
    status |= inv_imu_read_reg(APEX_CONFIG1, 1, (uint8_t*)&reg_apex_config1);
    reg_apex_config1.ff_en = APEX_CONFIG1_DMP_FF_EN;
    status |= inv_imu_write_reg(APEX_CONFIG1, 1, (uint8_t*)&reg_apex_config1);

	return status;
}

int inv_imu_disable_ff_register(void)
{
	int status = 0;
    apex_config1_t reg_apex_config1;
    int_source6_t reg_int_source6_t;
		
	 /* disable FF int1 */
	status |= inv_imu_read_reg(INT_SOURCE6_MREG, 1, (uint8_t*)&reg_int_source6_t);
    reg_int_source6_t.ff_int1_en = DIS;
    reg_int_source6_t.lowg_int1_en = DIS;
    status |= inv_imu_write_reg(INT_SOURCE6_MREG, 1, (uint8_t*)&reg_int_source6_t);

    /* disable FF */
    status |= inv_imu_read_reg(APEX_CONFIG1, 1, (uint8_t*)&reg_apex_config1);
    reg_apex_config1.ff_en = APEX_CONFIG1_DMP_FF_DIS;
    status |= inv_imu_write_reg(APEX_CONFIG1, 1, (uint8_t*)&reg_apex_config1);
	return status;
}

int inv_imu_apex_set_frequency(const APEX_CONFIG1_DMP_ODR_t frequency)
{
	int status = 0;
    apex_config1_t reg_apex_config1;
    
	status |= inv_imu_read_reg(APEX_CONFIG1, 1, (uint8_t*)&reg_apex_config1);
    
	reg_apex_config1.dmp_odr = frequency;

	status |= inv_imu_write_reg(APEX_CONFIG1, 1, (uint8_t*)&reg_apex_config1);
	return status;
}

int inv_imu_apex_init_parameters_struct( inv_imu_apex_parameters_t *apex_inputs)
{
	int status = 0;
	
	/* Default parameters at POR */
	apex_inputs->pedo_amp_th          = APEX_CONFIG3_PEDO_AMP_TH_2080374_MG;
	apex_inputs->pedo_step_cnt_th     = 0x5;
	apex_inputs->pedo_step_det_th     = APEX_CONFIG4_PEDO_SB_TIMER_TH_100_SAMPLES;
	apex_inputs->pedo_sb_timer_th     = APEX_CONFIG4_PEDO_SB_TIMER_TH_100_SAMPLES;
	apex_inputs->pedo_hi_enrgy_th     = APEX_CONFIG4_PEDO_HI_ENRGY_TH_107;
	apex_inputs->tilt_wait_time       = APEX_CONFIG5_TILT_WAIT_TIME_4S;
	apex_inputs->power_save_time      = APEX_CONFIG2_DMP_POWER_SAVE_TIME_SEL_8S;
	apex_inputs->sensitivity_mode     = APEX_CONFIG9_SENSITIVITY_MODE_NORMAL;
	apex_inputs->low_energy_amp_th    = APEX_CONFIG2_LOW_ENERGY_AMP_TH_SEL_2684354MG;
	apex_inputs->smd_sensitivity      = APEX_CONFIG9_SMD_SENSITIVITY_0;
	apex_inputs->ff_debounce_duration = APEX_CONFIG9_FF_DEBOUNCE_DURATION_2000_MS;
	apex_inputs->ff_max_duration_cm   = APEX_CONFIG12_FF_MAX_DURATION_204_CM;
	apex_inputs->ff_min_duration_cm   = APEX_CONFIG12_FF_MIN_DURATION_10_CM;
	apex_inputs->lowg_peak_th         = APEX_CONFIG10_LOWG_PEAK_TH_563MG;
	apex_inputs->lowg_peak_hyst       = APEX_CONFIG5_LOWG_PEAK_TH_HYST_156MG;
	apex_inputs->lowg_samples_th      = APEX_CONFIG10_LOWG_TIME_TH_5_SAMPLES;
	apex_inputs->highg_peak_th        = APEX_CONFIG11_HIGHG_PEAK_TH_2500MG;
	apex_inputs->highg_peak_hyst      = APEX_CONFIG5_HIGHG_PEAK_TH_HYST_156MG;
	apex_inputs->highg_samples_th     = APEX_CONFIG11_HIGHG_TIME_TH_1_SAMPLE;


	return status;
}

int inv_imu_apex_configure_parameters(const inv_imu_apex_parameters_t *apex_inputs)
{
	int status = 0;
	apex_config2_t reg_apex_config2;
    apex_config3_t reg_apex_config3;
    apex_config4_t reg_apex_config4;
    apex_config5_t reg_apex_config5;
    apex_config9_t reg_apex_config9;
    apex_config10_t reg_apex_config10;
    apex_config11_t reg_apex_config11;
    apex_config12_t reg_apex_config12;
    
	status |= inv_imu_switch_on_mclk();

	/* Power Save mode parameters & Additionnal parameters for Pedometer in Slow Walk mode */
	/* APEX_CONFIG2_MREG_TOP1 */
	reg_apex_config2.dmp_power_save_time_sel  = apex_inputs->power_save_time; 
	reg_apex_config2.low_energy_amp_th_sel    = apex_inputs->low_energy_amp_th;
    status |= inv_imu_write_reg(APEX_CONFIG2_MREG, 1, (uint8_t*)&reg_apex_config2 );
    
	/* Pedometer parameters */
	/* APEX_CONFIG3_MREG_TOP1 */
	reg_apex_config3.pedo_amp_th_sel =  apex_inputs->pedo_amp_th;
    reg_apex_config3.pedo_step_cnt_th_sel = apex_inputs->pedo_step_cnt_th;
    status |= inv_imu_write_reg(APEX_CONFIG3_MREG, 1, (uint8_t*)&reg_apex_config3 );

	/* APEX_CONFIG4_MREG_TOP1 */
	reg_apex_config4.pedo_step_det_th_sel = apex_inputs->pedo_step_det_th;
    reg_apex_config4.pedo_sb_timer_th_sel = apex_inputs->pedo_sb_timer_th;
    reg_apex_config4.pedo_hi_enrgy_th_sel = apex_inputs->pedo_hi_enrgy_th;
    status |= inv_imu_write_reg(APEX_CONFIG4_MREG, 1, (uint8_t*)&reg_apex_config4 );

	/* Tilt, Lowg and highg parameters */
	/* APEX_CONFIG5_MREG_TOP1 */
	reg_apex_config5.tilt_wait_time_sel  = apex_inputs->tilt_wait_time;
	reg_apex_config5.lowg_peak_th_hyst_sel =  apex_inputs->lowg_peak_hyst; 
    reg_apex_config5.highg_peak_th_hyst_sel =  apex_inputs->highg_peak_hyst;
    status |= inv_imu_write_reg(APEX_CONFIG5_MREG, 1, (uint8_t*)&reg_apex_config5 );
	
	/* free fall parameter, SMD parameter and parameters for Pedometer in Slow Walk mode */
	/* APEX_CONFIG9_MREG_TOP1 */
	reg_apex_config9.ff_debounce_duration_sel = apex_inputs->ff_debounce_duration; 
	reg_apex_config9.smd_sensitivity_sel      = apex_inputs->smd_sensitivity; 
    reg_apex_config9.sensitivity_mode         = apex_inputs->sensitivity_mode;
    status |= inv_imu_write_reg(APEX_CONFIG9_MREG, 1, (uint8_t*)&reg_apex_config9 );
    
	/* Lowg and highg parameters and free fall parameters */
	/* APEX_CONFIG10_MREG_TOP1 */
	reg_apex_config10.lowg_peak_th_sel = apex_inputs->lowg_peak_th;
    reg_apex_config10.lowg_time_th_sel = apex_inputs->lowg_samples_th;
    status |= inv_imu_write_reg(APEX_CONFIG10_MREG, 1, (uint8_t*)&reg_apex_config10 ); 

	/* APEX_CONFIG11_MREG_TOP1 */
	reg_apex_config11.highg_peak_th_sel = apex_inputs->highg_peak_th;
    reg_apex_config11.highg_time_th_sel = apex_inputs->highg_samples_th;
    status |= inv_imu_write_reg(APEX_CONFIG11_MREG, 1, (uint8_t*)&reg_apex_config11 ); 

	
	/* APEX_CONFIG12_MREG_TOP1 */
	reg_apex_config12.ff_max_duration_sel = apex_inputs->ff_max_duration_cm;
    reg_apex_config12.ff_min_duration_sel = apex_inputs->ff_min_duration_cm;
	
	status |= inv_imu_write_reg(APEX_CONFIG12_MREG, 1, (uint8_t*)&reg_apex_config12 ); 

	status |= inv_imu_switch_off_mclk();

	return status;
}

int inv_imu_apex_get_data_activity(inv_imu_apex_step_activity_t * apex_activity)
{
	uint8_t data[4];
	int status = inv_imu_read_reg(APEX_DATA0, 4, data);
	
	apex_activity->step_cnt = data[1] << 8 | data[0];
	apex_activity->step_cadence = data[2];
	apex_activity->activity_class = data[3] & APEX_DATA3_ACTIVITY_CLASS_MASK;
	
	return status;
}


int inv_imu_pedometer_get_event(float *cadence_step_per_sec,APEX_DATA3_ACTIVITY_CLASS_t *activity_class)
{
    int_status3_t reg_int_status3;
	int rc = 0;
    float nb_samples = 0;
    static uint64_t step_cnt = 0;
		/* 
	 * Read Pedometer interrupt status
	 */
	rc = inv_imu_read_reg(INT_STATUS3, 1, (uint8_t*)&reg_int_status3);
	if (rc != INV_ERROR_SUCCESS)
		return rc;
	
	if (reg_int_status3.step_det_int) {
		
		uint8_t step_cnt_ovflw = (reg_int_status3.step_cnt_ovf_int) ? 1 : 0;

		inv_imu_apex_step_activity_t apex_data0;
		rc |= inv_imu_apex_get_data_activity(&apex_data0);

		
		/* Converting u6.2 to float */
		nb_samples = (apex_data0.step_cadence >> 2) + (float)(apex_data0.step_cadence & 0x03)*0.25;
		*cadence_step_per_sec = (float) 50 / nb_samples;

		if (rc != INV_ERROR_SUCCESS)
			return rc;
		
		if (step_cnt == apex_data0.step_cnt + step_cnt_ovflw * (uint64_t)65536)
			return rc;

		step_cnt = apex_data0.step_cnt + step_cnt_ovflw * 65536;
        *activity_class = (APEX_DATA3_ACTIVITY_CLASS_t)apex_data0.activity_class;
        
		switch (apex_data0.activity_class) {
		case APEX_DATA3_ACTIVITY_CLASS_WALK:
			INV_LOG(SENSOR_LOG_LEVEL, "%d steps - cadence: %.2f steps/sec - WALK",  
				(uint32_t)step_cnt, 
				*cadence_step_per_sec);
			break;
		case APEX_DATA3_ACTIVITY_CLASS_RUN:
			INV_LOG(SENSOR_LOG_LEVEL, "%d steps - cadence: %.2f steps/sec - RUN", 
				(uint32_t)step_cnt, 
				*cadence_step_per_sec);
			break;
		default:
			INV_LOG(SENSOR_LOG_LEVEL, ": %d steps - cadence: %.2f steps/sec", 
				(uint32_t)step_cnt, 
				*cadence_step_per_sec);
			break;
		}
	}
	
    return step_cnt;
}

int inv_imu_pedometer_enable(void)
{
    int ret = 0;
    uint8_t regValue = 0;
    int odr_index = 0;
    uint32_t sampleRate = 0;

    if(icm_dev.min_apex_odr < SENSOR_HZ(50)){
        icm_dev.pre_min_apex_odr = icm_dev.min_apex_odr;
        icm_dev.min_apex_odr = SENSOR_HZ(50);
     }
    
     //record previous chip hardware_rate
    icm_dev.pre_hwRate = max(icm_dev.sensors[ACC].hwRate,icm_dev.sensors[GYR].hwRate);
        
    if(icm_dev.pre_hwRate < icm_dev.min_apex_odr) {
            //update new hardware rate 
            //icm4x6xx_reconfig_samplerate(mTask.hw_min_rate,mTask.hw_min_rate);

        // temp code we will use samplerate reconfig function later.
       icm_dev.sensors[ACC].hwRate = icm_dev.min_apex_odr;
       if(icm_dev.sensors[GYR].configed == true){
           icm_dev.sensors[GYR].hwRate = icm_dev.min_apex_odr;
       }
       odr_index = inv_imu_cal_odr(&icm_dev.min_apex_odr, &sampleRate);
       regValue = IMU_ODR_MAPPING[odr_index];
       INV_LOG(SENSOR_LOG_LEVEL, "set acc Odr Reg value 0x%x", regValue);
        /* update new odr */

       icm_dev.acc_cfg0.accel_odr = regValue;
       ret += inv_imu_write_reg(ACCEL_CONFIG0,1, (uint8_t*)&icm_dev.acc_cfg0);
       INV_LOG(SENSOR_LOG_LEVEL, "write ACCEL_CONFIG0  0x%x", icm_dev.acc_cfg0);

       if(icm_dev.sensors[GYR].configed == true){
           icm_dev.gyro_cfg0.gyro_odr = regValue;
           ret |= inv_imu_write_reg(GYRO_CONFIG0, 1, (uint8_t*)&icm_dev.gyro_cfg0);
           INV_LOG(SENSOR_LOG_LEVEL, "write GYRO_CONFIG0  0x%x", icm_dev.gyro_cfg0);    
       }

       inv_delay_us(200);
    }
 
    // if acc is not in streaming mode enable acc current work in LN mode first
    if((!icm_dev.sensors[ACC].configed) && (!icm_dev.sensors[GYR].configed))
        ret += inv_imu_enable_accel_low_power_mode(1024000000 / icm_dev.sensors[ACC].hwRate);  
    else
        ret += inv_imu_enable_accel_low_noise_mode(1024000000 / icm_dev.sensors[ACC].hwRate);   

   
    INV_LOG(SENSOR_LOG_LEVEL, "FF enable, acc on");

    
    if(false == icm_dev.dmp_is_on){
		 ret += inv_imu_dmp_powersave(DMP_POWERSAVE_PEDO);
	}  
    
    ret += inv_imu_apex_set_frequency(APEX_CONFIG1_DMP_ODR_50Hz);
    ret += inv_imu_start_dmp();
    ret += inv_imu_enable_pedo_register();
    
    if (ret != 0)
        return FF_ENABLE_ERROR;

    icm_dev.sensors[PEDO].powered = true;

    #if SENSOR_REG_DUMP
    inv_imu_dumpRegs();
    #endif
    
    return ret;
}

int inv_imu_pedometer_disable(void)
{
    int ret = 0;

    INV_LOG(SENSOR_LOG_LEVEL, "%s", __func__);
    
    ret |= inv_imu_disable_pedo_register();
    icm_dev.min_apex_odr = icm_dev.pre_min_apex_odr;
 
    if (false == icm_dev.sensors[ACC].powered && 
        false == icm_dev.sensors[ACC].configed && 
        false == icm_dev.sensors[WOM].powered &&
        false == icm_dev.sensors[PEDO].powered) {

        icm_dev.pwr_mgmt.accel_mode = PWR_MGMT_0_ACCEL_MODE_OFF;
        ret |= inv_imu_write_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);
        inv_delay_us(200); //spec: 200us
  
        INV_LOG(SENSOR_LOG_LEVEL, "wom disable, acc off pwr: 0x%x", icm_dev.pwr_mgmt);
    }
    
    icm_dev.sensors[FF].powered = false;
    
    return ret;
}


int inv_imu_freefall_enable(void)
{
    int ret = 0;
    uint8_t regValue = 0;
    int odr_index = 0;
    uint32_t sampleRate = 0;

    if(icm_dev.min_apex_odr < SENSOR_HZ(400)){
        icm_dev.pre_min_apex_odr = icm_dev.min_apex_odr;
        icm_dev.min_apex_odr = SENSOR_HZ(400);
     }
    
     //record previous chip hardware_rate
    icm_dev.pre_hwRate = max(icm_dev.sensors[ACC].hwRate,icm_dev.sensors[GYR].hwRate);
        
    if(icm_dev.pre_hwRate < icm_dev.min_apex_odr) {
            //update new hardware rate 
            //icm4x6xx_reconfig_samplerate(mTask.hw_min_rate,mTask.hw_min_rate);

        // temp code we will use samplerate reconfig function later.
       icm_dev.sensors[ACC].hwRate = icm_dev.min_apex_odr;
       if(icm_dev.sensors[GYR].configed == true){
           icm_dev.sensors[GYR].hwRate = icm_dev.min_apex_odr;
       }
       odr_index = inv_imu_cal_odr(&icm_dev.min_apex_odr, &sampleRate);
       regValue = IMU_ODR_MAPPING[odr_index];
       INV_LOG(SENSOR_LOG_LEVEL, "set acc Odr Reg value 0x%x", regValue);
        /* update new odr */

       icm_dev.acc_cfg0.accel_odr = regValue;
       ret += inv_imu_write_reg(ACCEL_CONFIG0,1, (uint8_t*)&icm_dev.acc_cfg0);
       INV_LOG(SENSOR_LOG_LEVEL, "write ACCEL_CONFIG0  0x%x", icm_dev.acc_cfg0);

       if(icm_dev.sensors[GYR].configed == true){
           icm_dev.gyro_cfg0.gyro_odr = regValue;
           ret |= inv_imu_write_reg(GYRO_CONFIG0, 1, (uint8_t*)&icm_dev.gyro_cfg0);
           INV_LOG(SENSOR_LOG_LEVEL, "write GYRO_CONFIG0  0x%x", icm_dev.gyro_cfg0);    
       }

       inv_delay_us(200);
    }
 
    // if acc is not in streaming mode enable acc current work in LN mode first
    if((!icm_dev.sensors[ACC].configed) && (!icm_dev.sensors[GYR].configed))
//        ret += inv_imu_enable_accel_low_power_mode(1024000000 / icm_dev.sensors[ACC].hwRate);  
//    else
        ret += inv_imu_enable_accel_low_noise_mode(1024000000 / icm_dev.sensors[ACC].hwRate);   

   
    INV_LOG(SENSOR_LOG_LEVEL, "FF enable, acc on");

    
    if(false == icm_dev.dmp_is_on){
		 ret += inv_imu_dmp_powersave(DMP_POWERSAVE_FF);
	}  
    
    ret += inv_imu_apex_set_frequency(APEX_CONFIG1_DMP_ODR_400Hz);
    ret += inv_imu_start_dmp();
    ret += inv_imu_enable_ff_register();
    
    if (ret != 0)
        return FF_ENABLE_ERROR;

    icm_dev.sensors[FF].powered = true;

    #if SENSOR_REG_DUMP
    inv_imu_dumpRegs();
    #endif
    
    return ret;
}

int inv_imu_freefall_disable(void)
{
    int ret = 0;

    INV_LOG(SENSOR_LOG_LEVEL, "%s", __func__);
    
    ret |= inv_imu_disable_ff_register();
    icm_dev.min_apex_odr = icm_dev.pre_min_apex_odr;
 
    if (false == icm_dev.sensors[ACC].powered && 
        false == icm_dev.sensors[ACC].configed && 
        false == icm_dev.sensors[WOM].powered &&
        false == icm_dev.sensors[PEDO].powered) {

        icm_dev.pwr_mgmt.accel_mode = PWR_MGMT_0_ACCEL_MODE_OFF;
        ret |= inv_imu_write_reg(PWR_MGMT_0, 1, (uint8_t*)&icm_dev.pwr_mgmt);
        inv_delay_us(200); //spec: 200us
  
        INV_LOG(SENSOR_LOG_LEVEL, "wom disable, acc off pwr: 0x%x", icm_dev.pwr_mgmt);
    }
    
    icm_dev.sensors[FF].powered = false;
    
    return ret;
}

float convert_ff_duration_sample_to_cm(uint16_t ff_duration_samples)
{
	int rc = 0;
	apex_config1_t reg_apex_config1;
	APEX_CONFIG1_DMP_ODR_t dmp_odr;
	uint32_t dmp_odr_us = 0;
	uint16_t ff_duration_ms;
	float ff_duration_cm;

	rc = inv_imu_read_reg(APEX_CONFIG1, 1, (uint8_t*)&reg_apex_config1);
    
	if (rc != 0)
		return -1;

	dmp_odr = (APEX_CONFIG1_DMP_ODR_t) (reg_apex_config1.dmp_odr);
	
	switch (dmp_odr) {
		case APEX_CONFIG1_DMP_ODR_25Hz:  dmp_odr_us = 40000; break;
		case APEX_CONFIG1_DMP_ODR_50Hz:  dmp_odr_us = 20000; break;
		case APEX_CONFIG1_DMP_ODR_100Hz: dmp_odr_us = 10000; break;
		case APEX_CONFIG1_DMP_ODR_400Hz: dmp_odr_us = 2500;  break;
	}
	
	ff_duration_ms = (ff_duration_samples * dmp_odr_us) / 1000;
	// dist = speed_average * time = (9.81 * time ^ 2) / 2)
	ff_duration_cm = (9.81 * ff_duration_ms * ff_duration_ms) / 2 / 10000 /* to cm/s^2 */;
	
	return ff_duration_cm;
}

float inv_imu_freefall_get_event(bool *ff_detect)
{

    float ff_duration_cm=0.0f;
    int_status3_t reg_int_status3;
	int rc = 0;
	uint16_t ff_duration_samples;
    uint8_t data[2];

    if (ff_detect == NULL) {
        INV_LOG(INV_LOG_LEVEL_ERROR, "EVENT NULL POINTER!!");
        return FF_GETEVENT_ERROR;
    }
	/*
	 *  Read FF interrupt status
	 */
	rc = inv_imu_read_reg(INT_STATUS3, 1, (uint8_t*)&reg_int_status3);
    
    if (rc != INV_ERROR_SUCCESS)
            return -1;

	if (reg_int_status3.lg_det_int) {
		INV_LOG(SENSOR_LOG_LEVEL, "low g detect") ;
	}

    if (reg_int_status3.ff_det_int) {
        rc |= inv_imu_read_reg(APEX_DATA4, 2, &data[0]);
		ff_duration_samples = (data[1] << 8) | data[0];
		
		ff_duration_cm = convert_ff_duration_sample_to_cm(ff_duration_samples);
		
		INV_LOG(SENSOR_LOG_LEVEL, " Duration of the fall : %.2f cm - %d samples", 
			ff_duration_cm, 
			ff_duration_samples);
		INV_LOG(SENSOR_LOG_LEVEL, "ff detect") ;

        *ff_detect = true;
	}

    return ff_duration_cm;
}


void inv_imu_dumpRegs()
{
    uint8_t data = 0;
    uint16_t size = 0;
	int i;
    uint32_t reg_map_bank_0[] = {
       DEVICE_CONFIG_REG,
        SIGNAL_PATH_RESET,
        INT_CONFIG_REG,
        PWR_MGMT_0,
        GYRO_CONFIG0,
        ACCEL_CONFIG0,
        TEMP_CONFIG0,
        GYRO_CONFIG1,
        ACCEL_CONFIG1,
        APEX_CONFIG0,
        APEX_CONFIG1,
        WOM_CONFIG,
        FIFO_CONFIG1,
        FIFO_CONFIG2,
        FIFO_CONFIG3,
        INT_SOURCE0,
        INT_SOURCE1,
        INTF_CONFIG0,
        INTF_CONFIG1
    };

    uint32_t reg_map_bank_top1[] = {
        TMST_CONFIG1_MREG,
        FIFO_CONFIG5_MREG,
        FIFO_CONFIG6_MREG,
        INT_SOURCE6_MREG,
        APEX_CONFIG2_MREG,
        APEX_CONFIG3_MREG,
        APEX_CONFIG4_MREG,
        APEX_CONFIG5_MREG,
        APEX_CONFIG9_MREG,
        APEX_CONFIG10_MREG,
        APEX_CONFIG11_MREG,
        APEX_CONFIG12_MREG,        
       };

    size = ARRAY_SIZE(reg_map_bank_0);
    for (i = 0; i < size; i++)
    {
        inv_imu_read_reg(reg_map_bank_0[i], 1, &data);
        INV_LOG(SENSOR_LOG_LEVEL, "bank0 reg[0x%x] 0x%x", reg_map_bank_0[i], data);
    }
    
    size = ARRAY_SIZE(reg_map_bank_top1);
    for (i = 0; i < size; i++)
    {
        inv_imu_read_reg(reg_map_bank_top1[i], 1, &data);
        INV_LOG(SENSOR_LOG_LEVEL, "top1 reg[0x%x] 0x%x", reg_map_bank_top1[i], data);
    }


}

