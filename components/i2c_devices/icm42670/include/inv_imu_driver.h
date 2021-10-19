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
#ifndef _INV_ICM4N607_H_
#define _INV_ICM4N607_H_

#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include "inv_imu_cust_config.h"


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


/* List whoami values for all icm436xx variants*/
#define ICM43607_WHOAMI 0x3F
#define ICM42607_WHOAMI 0x60
#define ICM42670_WHOAMI 0x67

#define T1000_WHOAMI    0x30

/* Register Map BANK0 */
#define MCLK_RDY                                                         0x0000

typedef struct{
  uint8_t otp_done              : 1;
  uint8_t resv_02               : 2;
  uint8_t mclk_rdy              : 1;
  uint8_t resv_01 			    : 4;
} mclk_rdy_t;

#define DEVICE_CONFIG_REG                                              0x0001

typedef struct{
  uint8_t spi_mode              : 1;
  uint8_t resv_01               : 1;
  uint8_t spi_ap_4wire          : 1;
  uint8_t resv_02               : 5;
} device_config_t;

#define SIGNAL_PATH_RESET                                              0x0002

typedef struct{
  uint8_t resv_01               : 2;
  uint8_t fifo_flush            : 1;
  uint8_t resv_02               : 1;
  uint8_t soft_reset_device_config      : 1;
  uint8_t resv_03               : 3;
} signal_path_reset_t;

#define DRIVE_CONFIG_REG1                                              0x0003

typedef struct{
  uint8_t i3c_sdr_slew_rate     : 3;
  uint8_t i3c_ddr_slew_rate     : 3;
  uint8_t resv_01               : 2;
} drive_config1_t;

#define DRIVE_CONFIG_REG2                                              0x0004

typedef struct{
  uint8_t all_slew_rate         : 3;
  uint8_t i2c_slew_rate         : 3;
  uint8_t resv_01               : 2;
} drive_config2_t;


#define DRIVE_CONFIG_REG3                                              0x0005

typedef struct{
  uint8_t SPI_slew_rate         : 3;
  uint8_t resv_01               : 5;
} drive_config3_t;

#define INT_CONFIG_REG                                                     0x0006

typedef struct{
  uint8_t int1_polarity         : 1;
  uint8_t int1_drive_circuit    : 1;
  uint8_t int1_mode             : 1;
  uint8_t int2_polarity         : 1;
  uint8_t int2_drive_circuit    : 1;
  uint8_t int2_mode             : 1;
  uint8_t resv_01               : 2;
} int_config_t;

#define TEMP_DATA1                                                     0x0009
#define TEMP_DATA0                                                     0x000a
#define ACCEL_DATA_X1                                                  0x000b
#define ACCEL_DATA_X0                                                  0x000c
#define ACCEL_DATA_Y1                                                  0x000d
#define ACCEL_DATA_Y0                                                  0x000e
#define ACCEL_DATA_Z1                                                  0x000f
#define ACCEL_DATA_Z0                                                  0x0010
#define GYRO_DATA_X1                                                   0x0011
#define GYRO_DATA_X0                                                   0x0012
#define GYRO_DATA_Y1                                                   0x0013
#define GYRO_DATA_Y0                                                   0x0014
#define GYRO_DATA_Z1                                                   0x0015
#define GYRO_DATA_Z0                                                   0x0016
#define TMST_FSYNCH                                                    0x0017
#define TMST_FSYNCL                                                    0x0018
#define APEX_DATA4                                                     0x001d
#define APEX_DATA5                                                     0x001e

#define PWR_MGMT_0                                                     0x001f

typedef struct{
  uint8_t accel_mode            : 2;
  uint8_t gyro_mode             : 2;
  uint8_t idle                  : 1;
  uint8_t resv_01               : 2;
  uint8_t accel_lp_clk_sel      : 1;
} pwr_mgmt0_t;

typedef union{
    uint8_t     data;
    pwr_mgmt0_t byte;
}PWR_MGMT0_T;

#define GYRO_CONFIG0                                                   0x0020

typedef struct{
  uint8_t gyro_odr              : 4;
  uint8_t resv01                : 1;
  uint8_t gyro_ui_fs_sel        : 2;
  uint8_t resv02                : 1;
} gyro_config0_t;

#define ACCEL_CONFIG0                                                  0x0021

typedef struct{
  uint8_t accel_odr             : 4;
  uint8_t bit4_resv             : 1;
  uint8_t accel_ui_fs_sel       : 2;
  uint8_t bit7_resv             : 1;
} accel_config0_t;

#define TEMP_CONFIG0                                                   0x0022

typedef struct{
  uint8_t resv_01               : 4;
  uint8_t temp_filt_bw          : 3;
  uint8_t resv_02               : 1;
} temp_config0_t;


#define GYRO_CONFIG1                                                   0x0023

typedef struct{
  uint8_t gyro_ui_filt_bw       : 3;
  uint8_t resv_01               : 5;
} gyro_config1_t;


#define ACCEL_CONFIG1                                                  0x0024

typedef struct{
  uint8_t accel_ui_filt_bw      : 3;
  uint8_t resv_01               : 1;
  uint8_t accel_ui_avg          : 3;
  uint8_t resv_02               : 1;
} accel_config1_t;


#define APEX_CONFIG0                                                   0x0025

typedef struct{
  uint8_t dmp_mem_reset_en      : 1;
  uint8_t resv_01               : 1;
  uint8_t dmp_init_en           : 1;
  uint8_t dmp_power_save_en     : 1;
  uint8_t resv_02               : 4;
} apex_config0_t;

#define APEX_CONFIG1                                                   0x0026

typedef struct{
  uint8_t dmp_odr               : 2;
  uint8_t resv_01               : 1;
  uint8_t ped_en                : 1;
  uint8_t tilt_en               : 1;
  uint8_t ff_en                 : 1;
  uint8_t smd_en                : 1;
  uint8_t resv_02               : 1;
} apex_config1_t;

#define WOM_CONFIG                                                     0x0027

typedef struct{
  uint8_t wom_en                : 1;
  uint8_t wom_mode              : 1;
  uint8_t wom_int_mode          : 1;
  uint8_t wom_int_dur           : 2;
  uint8_t resv_01               : 3;
} wom_config_t;

#define FIFO_CONFIG1                                                   0x0028

typedef struct{
  uint8_t fifo_bypass           : 1;
  uint8_t fifo_mode             : 1;
  uint8_t resv_01               : 6;
} fifo_config1_t;


#define FIFO_CONFIG2                                                   0x0029

typedef struct{
  uint8_t fifo_wm7_0            : 8;
} fifo_config2_t;

#define FIFO_CONFIG3                                                   0x002a

typedef struct{
  uint8_t fifo_wm11_8           : 4;
  uint8_t resv_01               : 4;
} fifo_config3_t;

#define INT_SOURCE0                                                    0x002b

typedef struct{
  uint8_t agc_rdy_int1_en       : 1;
  uint8_t fifo_full_int1_en     : 1;
  uint8_t fifo_ths_int1_en      : 1;
  uint8_t drdy_int1_en          : 1;
  uint8_t reset_done_int1_en    : 1;
  uint8_t pll_rdy_int1_en       : 1;
  uint8_t fsync_int1_en         : 1;
  uint8_t st_int1_en            : 1;
} int_source0_t;

#define INT_SOURCE1                                                    0x002c

typedef struct{
  uint8_t wom_x_int1_en         : 1;
  uint8_t wom_y_int1_en         : 1;
  uint8_t wom_z_int1_en         : 1;
  uint8_t smd_int1_en           : 1;
  uint8_t resv_01               : 2;
  uint8_t i3c_protocol_error_int1_en : 1;
  uint8_t resv_02               : 1;
} int_source1_t;

#define INT_SOURCE3                                                    0x002d

typedef struct{
  uint8_t agc_rdy_int2_en       : 1;
  uint8_t fifo_full_int2_en     : 1;
  uint8_t fifo_ths_int2_en      : 1;
  uint8_t drdy_int2_en          : 1;
  uint8_t reset_done_int2_en    : 1;
  uint8_t pll_rdy_int2_en       : 1;
  uint8_t fsync_int2_en         : 1;
  uint8_t st_int2_en            : 1;
} int_source3_t;

#define INT_SOURCE4                                                    0x002e

typedef struct{
  uint8_t wom_x_int2_en         : 1;
  uint8_t wom_y_int2_en         : 1;
  uint8_t wom_z_int2_en         : 1;
  uint8_t smd_int2_en           : 1;
  uint8_t resv_01               : 2;
  uint8_t i3c_protocol_error_int2_en: 1;
  uint8_t resv_02               : 1;
} int_sourc4_t;

#define FIFO_LOST_PKT0                                                 0x002f
#define FIFO_LOST_PKT1                                                 0x0030
#define APEX_DATA0                                                     0x0031
#define APEX_DATA1                                                     0x0032
#define APEX_DATA2                                                     0x0033

#define APEX_DATA3                                                     0x0034

typedef struct{
  uint8_t activity_class        : 2;
  uint8_t dmp_idle              : 1;
  uint8_t resv_01               : 5;
} apex_data3_t;

#define INTF_CONFIG0                                                   0x0035

typedef struct{
  uint8_t ui_sifs_cfg           : 2;
  uint8_t resv_01               : 2;
  uint8_t sensor_data_endian    : 1;
  uint8_t fifo_count_endian     : 1;
  uint8_t fifo_count_format     : 1;
  uint8_t resv_02               : 1;
} intf_config0_t;

#define INTF_CONFIG1                                                   0x0036

typedef struct{
  uint8_t clksel                : 2;
  uint8_t i3c_ddr_en            : 1;
  uint8_t i3c_sdr_en            : 1;
  uint8_t resv_01               : 4;
} intf_config1_t;

#define INT_STATUS_DRDY                                                0x0039

typedef struct{
  uint8_t data_rdy_int          : 1;
  uint8_t resv_01               : 7;
} int_status_drdy_t;


#define INT_STATUS                                                     0x003a

typedef struct{
  uint8_t agc_rdy_int           : 1;
  uint8_t fifo_full_int         : 1;
  uint8_t fifo_ths_int          : 1;
  uint8_t resv_01               : 1;
  uint8_t reset_done_int        : 1;
  uint8_t pll_rdy_int           : 1;
  uint8_t fsync_int             : 1;
  uint8_t st_int                : 1;
} int_status_t;

#define INT_STATUS2                                                    0x003b

typedef struct{
  uint8_t wom_z_int             : 1;
  uint8_t wom_y_int             : 1;
  uint8_t wom_x_int             : 1;
  uint8_t smd_int               : 1;
  uint8_t resv_01               : 4;
} int_status2_t;

#define INT_STATUS3                                                    0x003c

typedef struct{
  uint8_t resv_01               : 1;
  uint8_t lg_det_int            : 1;
  uint8_t ff_det_int            : 1;
  uint8_t tilt_det_int          : 1;
  uint8_t step_cnt_ovf_int      : 1;
  uint8_t step_det_int          : 1;
  uint8_t resv_02               : 2;
} int_status3_t;

#define FIFO_COUNTH                                                    0x003d
#define FIFO_COUNTL                                                    0x003e
#define FIFO_DATA                                                      0x003f
#define WHO_AM_I                                                       0x0075
#define BLK_SEL_W                                                      0x0079
#define MADDR_W                                                        0x007a
#define M_W                                                            0x007b
#define BLK_SEL_R                                                      0x007c
#define MADDR_R                                                        0x007d
#define M_R                                                            0x007e

/*end of common reg */

/* MMEM_SIGP */

/* MREG_TOP1 */
#define TMST_CONFIG1_MREG                                         0x1000

typedef struct{  
  uint8_t tmst_en               : 1;
  uint8_t tmst_fsyn_en          : 1;
  uint8_t tmst_delta_en         : 1;
  uint8_t tmst_res              : 1;
  uint8_t tmst_on_sreg_en       : 1;
  uint8_t resv_01               : 3;
} tmst_config1_t;

#define FIFO_CONFIG5_MREG                                         0x1001

typedef struct{
  uint8_t fifo_accel_en             : 1;
  uint8_t fifo_gyro_en              : 1;
  uint8_t fifo_tmst_fsync_en        : 1;
  uint8_t fifo_hires_en             : 1;
  uint8_t fifo_resume_partial_rd    : 1;
  uint8_t fifo_wm_gt_th             : 1;
  uint8_t resv_01                   : 2;
} fifo_config5_t;

#define FIFO_CONFIG6_MREG                                         0x1002
typedef struct{
  uint8_t rcosc_req_on_fifo_ths_dis  : 1;
  uint8_t resv_01                    : 3;
  uint8_t fifo_empty_indicator_dis   : 1;
  uint8_t resv_02                    : 3;
}fifo_config6_t;

#define FSYNC_CONFIG_MREG                                         0x1003
typedef struct{
  uint8_t fsync_polarity             : 1;
  uint8_t fsync_ui_flag_clear_sel    : 1;
  uint8_t resv_01                    : 2;
  uint8_t fsync_ui_sel               : 3;
  uint8_t resv_02                    : 1;
}fsync_config_t;


#define INT_CONFIG0_MREG                                          0x1004

#define INT_CONFIG1_MREG                                          0x1005
typedef struct{
  uint8_t resv_01                    : 4;
  uint8_t int_async_reset            : 1;
  uint8_t resv_02                    : 1;
  uint8_t int_tpulse_duration        : 1;
  uint8_t resv_03                    : 1;
}int_config1_t;

#define SENSOR_CONFIG3                                            0x1006
typedef struct{
  uint8_t resv_01                    : 6;
  uint8_t apex_disable               : 1;
  uint8_t resv_02                    : 1;
}sensor_config3_t;

#define ST_CONFIG_MREG                                            0x1013

typedef struct{  
  uint8_t gyro_st_lim                : 3;
  uint8_t accel_st_lim               : 3;
  uint8_t st_num_sample              : 1;
  uint8_t resv_01                    : 1;
} st_config_t;

#define SELFTEST_MREG                                             0x1014

typedef struct{
  uint8_t resv_01                    : 6;
  uint8_t accel_st_en                : 1;
  uint8_t gyro_st_en                 : 1;
} selftest_t;

#define INTF_CONFIG6_MREG                                         0x1023
#define INTF_CONFIG10_MREG                                        0x1025
#define INTF_CONFIG7_MREG                                         0x1028

#define OTP_CONFIG                                                0x102b

typedef struct{
  uint8_t resv_01                     : 2;
  uint8_t otp_copy_mode               : 2;
  uint8_t resv_02                     : 4;
} otp_config_t;


#define INT_SOURCE6_MREG                                          0x102f

typedef struct{
  uint8_t resv_01                    : 3;
  uint8_t tilt_det_int1_en       : 1;
  uint8_t step_cnt_ovfl_int1_en  : 1;
  uint8_t step_det_int1_en       : 1;
  uint8_t lowg_int1_en           : 1;
  uint8_t ff_int1_en             : 1;
} int_source6_t;

#define INT_SOURCE7_MREG                                          0x1030

typedef struct{
  uint8_t resv_01                    : 3;
  uint8_t tilt_det_int2_en       : 1;
  uint8_t step_cnt_ovfl_int2_en  : 1;
  uint8_t step_det_int2_en       : 1;
  uint8_t lowg_int2_en           : 1;
  uint8_t ff_int2_en             : 1;
} int_source7_t;

#define INT_SOURCE8_MREG                                          0x1031
#define INT_SOURCE9_MREG                                          0x1032
#define INT_SOURCE10_MREG                                         0x1033

#define APEX_CONFIG2_MREG                                         0x1044

typedef struct{
  uint8_t dmp_power_save_time_sel     : 4;
  uint8_t low_energy_amp_th_sel       : 4;
} apex_config2_t;

#define APEX_CONFIG3_MREG                                         0x1045

typedef struct{
  uint8_t pedo_step_cnt_th_sel        : 4;
  uint8_t pedo_amp_th_sel             : 4;
} apex_config3_t;

#define APEX_CONFIG4_MREG                                         0x1046

typedef struct{
  uint8_t pedo_hi_enrgy_th_sel        : 2;
  uint8_t pedo_sb_timer_th_sel        : 3;
  uint8_t pedo_step_det_th_sel        : 3; 
} apex_config4_t;

#define APEX_CONFIG5_MREG                                         0x1047

typedef struct{
  uint8_t highg_peak_th_hyst_sel        : 3;
  uint8_t lowg_peak_th_hyst_sel         : 3;
  uint8_t tilt_wait_time_sel            : 2; 
} apex_config5_t;

#define APEX_CONFIG9_MREG                                         0x1048

typedef struct{
  uint8_t sensitivity_mode             : 1;
  uint8_t smd_sensitivity_sel          : 3;
  uint8_t ff_debounce_duration_sel     : 4; 
} apex_config9_t;

#define APEX_CONFIG10_MREG                                        0x1049

typedef struct{
  uint8_t lowg_time_th_sel             : 3;
  uint8_t lowg_peak_th_sel             : 5;
} apex_config10_t;

#define APEX_CONFIG11_MREG                                        0x104a

typedef struct{
  uint8_t highg_time_th_sel             : 3;
  uint8_t highg_peak_th_sel             : 5;
} apex_config11_t;

#define ACCEL_WOM_X_THR_MREG                                      0x104b
#define ACCEL_WOM_Y_THR_MREG                                      0x104c
#define ACCEL_WOM_Z_THR_MREG                                      0x104d
#define OFFSET_USER0                                              0x104e
#define OFFSET_USER1                                              0x104f
#define OFFSET_USER2                                              0x1050
#define OFFSET_USER3                                              0x1051
#define OFFSET_USER4                                              0x1052
#define OFFSET_USER5                                              0x1053
#define OFFSET_USER6                                              0x1054
#define OFFSET_USER7                                              0x1055
#define OFFSET_USER8                                              0x1056

#define ST_STATUS1_MREG                                           0x1063
#define FDR_CONFIG_MREG                                           0x1066


typedef struct{
  uint8_t resv_01               : 1;
  uint8_t ax_st_pass            : 1;
  uint8_t ay_st_pass            : 1;
  uint8_t az_st_pass            : 1;
  uint8_t accel_st_done         : 1;
  uint8_t accel_st_pass         : 1;
  uint8_t resv_02               : 2;
} st_status1_t;

#define ST_STATUS2_MREG                                           0x1064

typedef struct{
  uint8_t resv_01               : 1;
  uint8_t gx_st_pass            : 1;
  uint8_t gy_st_pass            : 1;
  uint8_t gz_st_pass            : 1;
  uint8_t gyro_st_done          : 1;
  uint8_t gyro_st_pass          : 1;
  uint8_t st_incomplete         : 1;
  uint8_t resv_02               : 1;
} st_status2_t;


#define APEX_CONFIG12_MREG                                        0x1067

typedef struct{
  uint8_t ff_min_duration_sel       : 4;
  uint8_t ff_max_duration_sel       : 4;
}apex_config12_t;


/* MMEM_TOP */
#define XA_ST_DATA_MMEM                                            0x5000
#define YA_ST_DATA_MMEM                                            0x5001
#define ZA_ST_DATA_MMEM                                            0x5002
#define XG_ST_DATA_MMEM                                            0x5003
#define YG_ST_DATA_MMEM                                            0x5004
#define ZG_ST_DATA_MMEM                                            0x5005

/* MREG_OTP */
/* Bank MREG2 */
#define OTP_CTRL7                                        0x2806

typedef struct{
  uint8_t otp_soak                : 1;
  uint8_t otp_pwr_down            : 1;
  uint8_t otp_low_pwr_mode        : 1;
  uint8_t otp_reload              : 1;
  uint8_t otp_boot                : 1;
  uint8_t otp_stress              : 1;
  uint8_t otp_reset               : 1;
  uint8_t pgm_dly_cfg             : 1;
} otp_ctrl7_t;



/* ---------------------------------------------------------------------------
 * register bank 0 
 * ---------------------------------------------------------------------------- */

/*
 * MISC_1 (MCLK_RDY register in datasheet)
 * Register Name : MISC_1 (MCLK_RDY)
 */

/*
 * mclk_rdy 
 * 1 to indicate internal clock is currently running. 
 * 0 to indicate internal clock is not currently running. 
 */
 typedef enum {
	MCLK_RDY_YES  = (1 ),
	MCLK_RDY_NO   = (0 ),
} MCLK_RDY_t;

/*
 * otp_done 
 * 1 to indicate OTP_copy operation is done.
 */
 typedef enum {
	OTP_DONE_YES  = (1 ),
    OTP_DONE_NO   = (0 ),
} OTP_DONE_t;


/*
 * DEVICE_CONFIG
 * Register Name : DEVICE_CONFIG
 */

/* SPI_MODE */
typedef enum
{
	DEVICE_CONFIG_SPI_MODE_1_2 = (0x1 ),
	DEVICE_CONFIG_SPI_MODE_0_3 = (0x0 ),
} DEVICE_CONFIG_SPI_MODE_t;

/* SPI_AP_4WIRE */
typedef enum
{
	DEVICE_CONFIG_SPI_AP_4WIRE = (0x1 ),
	DEVICE_CONFIG_SPI_AP_3WIRE = (0x0 ),
} DEVICE_CONFIG_SPI_AP_4WIRE_t;


/*
 * SIGNAL_PATH_RESET
 * Register Name: SIGNAL_PATH_RESET
 */
/* SOFT_RESET_DEVICE_CONFIG */
typedef enum
{
    SIGNAL_PATH_RESET_SOFT_RESET_DEVICE_CONFIG_EN  = (0x01 ),
	SIGNAL_PATH_RESET_SOFT_RESET_DEVICE_CONFIG_DIS = (0x00 ),
} SIGNAL_PATH_RESET_SOFT_RESET_DEVICE_CONFIG_t;

/*
 * FIFO_FLUSH 
 * When set to 1, FIFO will get flushed. FIFO flush requires special sequence of programming the registers. Please see use notes for details
 * Host can only program this register bit to 1. */
typedef enum
{
	SIGNAL_PATH_RESET_FIFO_FLUSH_EN  = (0x01 ),
	SIGNAL_PATH_RESET_FIFO_FLUSH_DIS = (0x00 ),
} SIGNAL_PATH_RESET_FIFO_FLUSH_t;


/*
 * FIFO_CONFIG1
 * Register Name: FIFO_CONFIG
 */

/* FIFO_BYPASS */
typedef enum
{
	FIFO_CONFIG1_FIFO_BYPASS_ON    = (0x01 ),
	FIFO_CONFIG1_FIFO_BYPASS_OFF   = (0x00 ),
} FIFO_CONFIG1_FIFO_BYPASS_t;

typedef enum
{
	FIFO_CONFIG1_FIFO_MODE_SNAPSHOT  = (0x01 ),
	FIFO_CONFIG1_FIFO_MODE_STREAM    = (0x00 ),
} FIFO_CONFIG1_FIFO_MODE_t;

/** Contains Pedometer output */
typedef struct APEX_DATA_STEP_ACTIVITY {
	uint16_t step_cnt;      /**< Number of steps taken */
	uint8_t step_cadence;   /**< Walk/run cadency in number of samples. 
	                             Format is u6.2. E.g, At 50Hz and 2Hz walk frequency, if the cadency is 25 samples. The register will output 100. */
	uint8_t activity_class; /**< Detected activity unknown (0), walk (1) or run (2) */
} APEX_DATA_STEP_ACTIVITY_t;

/* ACTIVITY_CLASS */
typedef enum
{
	APEX_DATA3_ACTIVITY_CLASS_OTHER = 0x0,
	APEX_DATA3_ACTIVITY_CLASS_WALK  = 0x1,
	APEX_DATA3_ACTIVITY_CLASS_RUN   = 0x2,
} APEX_DATA3_ACTIVITY_CLASS_t;

/* DMP_IDLE */
typedef enum
{
	APEX_DATA3_DMP_IDLE_ON     = (0x01 ),
	APEX_DATA3_DMP_IDLE_OFF    = (0x00 ),
} APEX_DATA3_DMP_IDLE_OFF_t;

/*
 * INTF_CONFIG0
 * Register Name: INTF_CONFIG0
 */

/* FIFO_SREG_INVALID_IND */
typedef enum
{
	INTF_CONFIG0_FIFO_SREG_INVALID_IND_DIS = (0x01 ),
	INTF_CONFIG0_FIFO_SREG_INVALID_IND_EN  = (0x00 ),
} INTF_CONFIG0_FIFO_SREG_INVALID_IND_t;

/* FIFO_COUNT_FORMAT */
typedef enum
{
	INTF_CONFIG0_FIFO_COUNT_FORMAT_RECORD = (0x01 ),
	INTF_CONFIG0_FIFO_COUNT_FORMAT_BYTE   = (0x00 ),
} INTF_CONFIG0_FIFO_COUNT_FORMAT_t;

/* FIFO_COUNT_ENDIAN */
typedef enum
{
	INTF_CONFIG0_FIFO_COUNT_BIG_ENDIAN    = (0x01 ),
	INTF_CONFIG0_FIFO_COUNT_LITTLE_ENDIAN = (0x00 ),
} INTF_CONFIG0_FIFO_COUNT_ENDIAN_t;

/* SENSOR_DATA_ENDIAN */
typedef enum
{
	INTF_CONFIG0_SENSOR_DATA_BIG_ENDIAN    = (0x01 ),
	INTF_CONFIG0_SENSOR_DATA_LITTLE_ENDIAN = (0x00 ),
} INTF_CONFIG0_SENSOR_DATA_ENDIAN_t;

/* UI_SIFS_CFG */
typedef enum
{
	INTF_CONFIG0_UI_SIFS_CFG_DISABLE_SPI   = (0x02 ),
	INTF_CONFIG0_UI_SIFS_CFG_DISABLE_I2C   = (0x03 ),
} INTF_CONFIG0_UI_SIFS_CFG_t;


/*
 * INTF_CONFIG1
 * Register Name: INTF_CONFIG1
 */

/* I3C_SDR_EN 
 * Enable I3C SDR mode. Chip will be in pure I2C mode if {i3c_sdr_en, i3c_ddr_en} = 00.
 */
typedef enum
{
	INTF_CONFIG1_I3C_SDR_EN    = (0x01 ),
	INTF_CONFIG1_I3C_SDR_DIS   = (0x00 ),
} INTF_CONFIG1_I3C_SDR_EN_t;

/* I3C_DDR_EN 
 * Enable I3C DDR mode. This bit won't take effect unless i3c_sdr_en = 1.
 */
typedef enum
{
	INTF_CONFIG1_I3C_DDR_EN    = (0x01 ),
	INTF_CONFIG1_I3C_DDR_DIS   = (0x00 ),
} INTF_CONFIG1_I3C_DDR_EN_t;

/* I3C_CLKSEL */
typedef enum
{
	INTF_CONFIG1_CLKSEL_RCOSC     = (0x00 ),
	INTF_CONFIG1_CLKSEL_PLL_RCOSC = (0x01 ),
	INTF_CONFIG1_CLKSEL_DIS_ALL   = (0x03 ),	
} INTF_CONFIG1_CLKSEL_t;


/*
 * INT_STATUS_DRDY
 * Register Name : INT_STATUS_DRDY
 * This is a clear on read register.
 * DRDY_DATA_REG (Accel / Gyro)
 */
typedef enum
{
	INT_STATUS_DRDY_DATA_RDY_INT_GENERATED     = (0x01 ),
	INT_STATUS_DRDY_DATA_RDY_INT_CLEARED       = (0x00 ),
} INT_STATUS_DRDY_DATA_RDY_INT_t;


/*
 * INT_STATUS
 * Register Name : INT_STATUS
 */

/*
 * int_status_st_done 
 * Self-Test Done
 */
typedef enum
{
	INT_STATUS_ST_INT_GENERATED     = (0x01 ),
	INT_STATUS_ST_INT_CLEARED       = (0x00 ),
} INT_STATUS_ST_INT_t;

/*
 * int_status_fsync_int
 * FSYNC_INT
 */
typedef enum
{
	INT_STATUS_FSYNC_INT_GENERATED     = (0x01 ),
	INT_STATUS_FSYNC_INT_CLEARED       = (0x00 ),
} INT_STATUS_FSYNC_INT_t;

/*
 * int_status_pll_rdy_int 
 * PLL_RDY_INT
 */
typedef enum
{
	INT_STATUS_PLL_RDY_INT_GENERATED     = (0x01 ),
	INT_STATUS_PLL_RDY_INT_CLEARED       = (0x00 ),
} INT_STATUS_PLL_RDY_INT_t;

/*
 * int_status_reset_done_int 
 * This is a clear on read register.
 * POR complete and Software reset complete
 */
typedef enum
{
	INT_STATUS_RESET_DONE_INT_GENERATED     = (0x01 ),
	INT_STATUS_RESET_DONE_INT_CLEARED       = (0x00 ),
} INT_STATUS_RESET_DONE_INT_t;

/*
 * int_status_fifo_ths_int 
 * This is a clear on read register.
 * FIFO Threshold
 */
typedef enum
{
	INT_STATUS_FIFO_THS_INT_GENERATED     = (0x01 ),
	INT_STATUS_FIFO_THS_INT_CLEARED       = (0x00 ),
} INT_STATUS_FIFO_THS_INT_t;

/*
 * int_status_fifo_full_int 
 * This is a clear on read register.
 * FIFO Full
 */
typedef enum
{
	INT_STATUS_FIFO_FULL_INT_GENERATED     = (0x01 ),
	INT_STATUS_FIFO_FULL_INT_CLEARED       = (0x00 ),
} INT_STATUS_FIFO_FULL_INT_t;

/*
 * int_status_agc_rdy_int 
 * This is a clear on read register.
 * Gyro drive is ready
 */
typedef enum
{
	INT_STATUS_AGC_RDY_INT_GENERATED     = (0x01 ),
	INT_STATUS_AGC_RDY_INT_CLEARED       = (0x00 ),
} INT_STATUS_AGC_RDY_INT_t;


/* INT_STATUS2 */

/*
 * int_status_smd_int 
 * SMD Interrupt clears on read
 */
typedef enum
{
	INT_STATUS2_SMD_INT_GENERATED     = (0x01 ),
	INT_STATUS2_SMD_INT_CLEARED       = (0x00 ),
} INT_STATUS2_SMD_INT_t;

/*
 * int_status_wom_x 
 * WOM Interrupt on X axis, clears on read
 */
typedef enum
{
	INT_STATUS2_WOM_X_INT_GENERATED     = (0x01 ),
	INT_STATUS2_WOM_X_INT_CLEARED       = (0x00 ),
} INT_STATUS2_WOM_X_INT_t;

/*
 * int_status_wom_y 
 * WOM Interrupt on Y axis, clears on read
 */
typedef enum
{
	INT_STATUS2_WOM_Y_INT_GENERATED     = (0x01 ),
	INT_STATUS2_WOM_Y_INT_CLEARED       = (0x00 ),
} INT_STATUS2_WOM_Y_INT_t;

/*
 * int_status_wom_z 
 * WOM interrupt on Z axis , clears on read
 */
typedef enum
{
	INT_STATUS2_WOM_Z_INT_GENERATED     = (0x01 ),
	INT_STATUS2_WOM_Z_INT_CLEARED       = (0x00 ),
} INT_STATUS2_WOM_Z_INT_t;


/*
 * PWR_MGMT_0
 * Register Name: PWR_MGMT_0
 */

/* ACCEL_LP_CLK_SEL */
typedef enum
{
	PWR_MGMT_0_ACCEL_LP_CLK_WUOSC = (0x00 ),
	PWR_MGMT_0_ACCEL_LP_CLK_RCOSC = (0x01 ),
} PWR_MGMT_0_ACCEL_LP_CLK_t;

/* IDLE */
typedef enum
{
	PWR_MGMT_0_IDLE_RCOSC_ON = (0x01 ),
	PWR_MGMT_0_IDLE_RCOSC_OFF  = (0x00 ),
} PWR_MGMT_0_IDLE_t;
 
/* GYRO_MODE */
typedef enum
{
	PWR_MGMT_0_GYRO_MODE_LN      = (0x03 ),
	PWR_MGMT_0_GYRO_MODE_LP      = (0x02 ),
	PWR_MGMT_0_GYRO_MODE_STANDBY = (0x01 ),
	PWR_MGMT_0_GYRO_MODE_OFF     = (0x00 ),
} PWR_MGMT_0_GYRO_MODE_t;

/* ACCEL_MODE */
typedef enum
{
	PWR_MGMT_0_ACCEL_MODE_LN  = 0x03,
	PWR_MGMT_0_ACCEL_MODE_LP  = 0x02,
	PWR_MGMT_0_ACCEL_MODE_OFF = 0x00,
} PWR_MGMT_0_ACCEL_MODE_t;


/*
 * GYRO_CONFIG0
 * Register Name: GYRO_CONFIG0
 */

/* GYRO_FS_SEL*/

/** @brief Gyroscope FSR selection 
 */
typedef enum
{
#if defined(ICM43688)
	GYRO_CONFIG0_FS_SEL_500dps  = (3 ),  /*!< 500dps*/
	GYRO_CONFIG0_FS_SEL_1000dps = (2 ),  /*!< 1000dps*/
	GYRO_CONFIG0_FS_SEL_2000dps = (1 ),  /*!< 2000dps*/
	GYRO_CONFIG0_FS_SEL_4000dps = (0 ),  /*!< 4000dps*/
#else
	GYRO_CONFIG0_FS_SEL_250dps  = (3 ),  /*!< 250dps*/
	GYRO_CONFIG0_FS_SEL_500dps  = (2 ),  /*!< 500dps*/
	GYRO_CONFIG0_FS_SEL_1000dps = (1 ),  /*!< 1000dps*/
	GYRO_CONFIG0_FS_SEL_2000dps = (0 ),  /*!< 2000dps*/
#endif
} GYRO_CONFIG0_FS_SEL_t;

/* GYRO_ODR */

/** @brief Gyroscope ODR selection 
 */
typedef enum
{
	GYRO_CONFIG0_ODR_1_5625_HZ = 0xF,  /*!< 1.5625 Hz (640 ms)*/
	GYRO_CONFIG0_ODR_3_125_HZ  = 0xE,  /*!< 3.125 Hz (320 ms)*/
	GYRO_CONFIG0_ODR_6_25_HZ   = 0xD,  /*!< 6.25 Hz (160 ms)*/
	GYRO_CONFIG0_ODR_12_5_HZ   = 0xC,  /*!< 12.5 Hz (80 ms)*/
	GYRO_CONFIG0_ODR_25_HZ     = 0xB,  /*!< 25 Hz (40 ms)*/
	GYRO_CONFIG0_ODR_50_HZ     = 0xA,  /*!< 50 Hz (20 ms)*/
	GYRO_CONFIG0_ODR_100_HZ    = 0x9,  /*!< 100 Hz (10 ms)*/
	GYRO_CONFIG0_ODR_200_HZ    = 0x8,  /*!< 200 Hz (5 ms)*/
	GYRO_CONFIG0_ODR_400_HZ    = 0x7,  /*!< 400 Hz (2.5 ms)*/
	GYRO_CONFIG0_ODR_800_HZ    = 0x6,  /*!< 800 Hz (1.25 ms)*/
	GYRO_CONFIG0_ODR_1600_HZ   = 0x5,  /*!< 1.6 KHz (625 us)*/
} GYRO_CONFIG0_ODR_t;

/*
 * ACCEL_CONFIG0
 * Register Name: ACCEL_CONFIG0
 */

/* ACCEL_FS_SEL */

/** @brief Accelerometer FSR selection 
 */
typedef enum
{
#if defined(ICM43688)
	ACCEL_CONFIG0_FS_SEL_4g  = (0x3 ),  /*!< 4g*/
	ACCEL_CONFIG0_FS_SEL_8g  = (0x2 ),  /*!< 8g*/
	ACCEL_CONFIG0_FS_SEL_16g = (0x1 ),  /*!< 16g*/
	ACCEL_CONFIG0_FS_SEL_32g = (0x0 ),  /*!< 32g*/
#else
	ACCEL_CONFIG0_FS_SEL_2g  = (0x3 ),  /*!< 2g*/
	ACCEL_CONFIG0_FS_SEL_4g  = (0x2 ),  /*!< 4g*/
	ACCEL_CONFIG0_FS_SEL_8g  = (0x1 ),  /*!< 8g*/
	ACCEL_CONFIG0_FS_SEL_16g = (0x0 ),  /*!< 16g*/
#endif
} ACCEL_CONFIG0_FS_SEL_t;

/* ACCEL_ODR */

/** @brief Accelerometer ODR selection 
 */
typedef enum
{
	ACCEL_CONFIG0_ODR_1_5625_HZ = 0xF,  /*!< 1.5625 Hz (640 ms)*/
	ACCEL_CONFIG0_ODR_3_125_HZ  = 0xE,  /*!< 3.125 Hz (320 ms)*/
	ACCEL_CONFIG0_ODR_6_25_HZ   = 0xD,  /*!< 6.25 Hz (160 ms)*/
	ACCEL_CONFIG0_ODR_12_5_HZ   = 0xC,  /*!< 12.5 Hz (80 ms)*/
	ACCEL_CONFIG0_ODR_25_HZ     = 0xB,  /*!< 25 Hz (40 ms)*/
	ACCEL_CONFIG0_ODR_50_HZ     = 0xA,  /*!< 50 Hz (20 ms)*/
	ACCEL_CONFIG0_ODR_100_HZ    = 0x9,  /*!< 100 Hz (10 ms)*/
	ACCEL_CONFIG0_ODR_200_HZ    = 0x8,  /*!< 200 Hz (5 ms)*/
	ACCEL_CONFIG0_ODR_400_HZ    = 0x7,  /*!< 400 Hz (2.5 ms)*/
	ACCEL_CONFIG0_ODR_800_HZ    = 0x6,  /*!< 800 Hz (1.25 ms)*/
	ACCEL_CONFIG0_ODR_1600_HZ   = 0x5,  /*!< 1.6 KHz (625 us)*/
} ACCEL_CONFIG0_ODR_t;

/*
 * GYRO_CONFIG1
 * Register Name: GYRO_CONFIG1
 */

/* GYRO_UI_FILT_BW_IND */
typedef enum
{
	GYRO_CONFIG1_GYRO_FILT_BW_16        = (0x07 ),
	GYRO_CONFIG1_GYRO_FILT_BW_25        = (0x06 ),
	GYRO_CONFIG1_GYRO_FILT_BW_34        = (0x05 ),
	GYRO_CONFIG1_GYRO_FILT_BW_53        = (0x04 ),
	GYRO_CONFIG1_GYRO_FILT_BW_73        = (0x03 ),
	GYRO_CONFIG1_GYRO_FILT_BW_121       = (0x02 ),
	GYRO_CONFIG1_GYRO_FILT_BW_180       = (0x01 ),
	GYRO_CONFIG1_GYRO_FILT_BW_NO_FILTER = (0x00 ),
} GYRO_CONFIG1_GYRO_FILT_BW_t;

/* GYRO_UI_AVG_IND */
typedef enum
{
	GYRO_CONFIG1_GYRO_FILT_AVG_64  = (0x05 ),
	GYRO_CONFIG1_GYRO_FILT_AVG_32  = (0x04 ),
	GYRO_CONFIG1_GYRO_FILT_AVG_16  = (0x03 ),
	GYRO_CONFIG1_GYRO_FILT_AVG_8   = (0x02 ),
	GYRO_CONFIG1_GYRO_FILT_AVG_4   = (0x01 ),
	GYRO_CONFIG1_GYRO_FILT_AVG_2   = (0x00 ),
} GYRO_CONFIG1_GYRO_FILT_AVG_t;

/*
 * ACCEL_CONFIG1
 * Register Name: ACCEL_CONFIG1
 */

/* ACCEL_UI_FILT_BW_IND */
typedef enum
{
	ACCEL_CONFIG1_ACCEL_FILT_BW_16        = (0x7 ),
	ACCEL_CONFIG1_ACCEL_FILT_BW_25        = (0x6 ),
	ACCEL_CONFIG1_ACCEL_FILT_BW_34        = (0x5 ),
	ACCEL_CONFIG1_ACCEL_FILT_BW_53        = (0x4 ),
	ACCEL_CONFIG1_ACCEL_FILT_BW_73        = (0x3 ),
	ACCEL_CONFIG1_ACCEL_FILT_BW_121       = (0x2 ),
	ACCEL_CONFIG1_ACCEL_FILT_BW_180       = (0x1 ),
	ACCEL_CONFIG1_ACCEL_FILT_BW_NO_FILTER = (0x0 ),
} ACCEL_CONFIG1_ACCEL_FILT_BW_t;

/* ACCEL_UI_AVG_IND */
typedef enum
{
	ACCEL_CONFIG1_ACCEL_FILT_AVG_64  = (0x5 ),
	ACCEL_CONFIG1_ACCEL_FILT_AVG_32  = (0x4 ),
	ACCEL_CONFIG1_ACCEL_FILT_AVG_16  = (0x3 ),
	ACCEL_CONFIG1_ACCEL_FILT_AVG_8   = (0x2 ),
	ACCEL_CONFIG1_ACCEL_FILT_AVG_4   = (0x1 ),
	ACCEL_CONFIG1_ACCEL_FILT_AVG_2   = (0x0 ),
} ACCEL_CONFIG1_ACCEL_FILT_AVG_t;

/*
 * GYRO_CONFIG1
 * Register Name: GYRO_CONFIG1
 */


/*
 * APEX_CONFIG0
 * Register Name: APEX_CONFIG0
 */
/*
 * DMP_MEM_RESET_EN
 * enable DMP SRAM to be cleared to 0.
 * [0]: 1 to clear DMP SRAM for APEX operation or Self-test operation.
 * [1]: 1 to clear DMP SRAM for secure Authentication operation. Secure authetication is not supported in Xian so this bit will be un-used.
 * This register field is clearred to 0 after SRAM contents are cleared. 
 */
typedef enum
{
	APEX_CONFIG0_DMP_MEM_RESET_EN  = (0x01 ),
	APEX_CONFIG0_DMP_MEM_RESET_DIS = (0x00 ),
} APEX_CONFIG0_DMP_MEM_RESET_EN_t;

/* DMP_INIT_EN
 * Enable Init algorithm. It can only be set to 1. Once this bit is set, the algorithm must be executed in the next ODR with highest priority than the other features. 
 * The readback value is the internal signal resynchronized bit that once falling back to zero indicates that the algorithm has finished.
 * This field can be changed on-the-fly even if accel sensor is already on.
 */
typedef enum
{
	APEX_CONFIG0_DMP_INIT_EN  = (0x01 ),
	APEX_CONFIG0_DMP_INIT_DIS = (0x00 ),
} APEX_CONFIG0_DMP_INIT_t;

/* DMP_POWER_SAVE_EN
 * Enable Power Saving for DMP algorithms.  DMP block have 16-bit power_save_en. This control bit will be connected to LSB bit of dmp_power_save_en which dm_power_save_en[0]
 */
 typedef enum
{
	APEX_CONFIG0_DMP_POWER_SAVE_EN   = (0x1 ),
	APEX_CONFIG0_DMP_POWER_SAVE_DIS  = (0x0 ),
} APEX_CONFIG0_DMP_POWER_SAVE_t;


/*
 * APEX_CONFIG1
 * Register Name: APEX_CONFIG1
 */

/* PEDO_EN */
typedef enum
{
	APEX_CONFIG1_PEDO_EN_EN   = (0x1 ),
	APEX_CONFIG1_PEDO_EN_DIS  = (0x0 ),
} APEX_CONFIG1_PEDO_EN_t;

/* TILT_EN */
typedef enum
{
	APEX_CONFIG1_TILT_EN_EN   = (0x1 ),
	APEX_CONFIG1_TILT_EN_DIS  = (0x0 ),
} APEX_CONFIG1_TILT_EN_t;

/* DMP_ODR */
/** @brief DMP ODR selection
 */
typedef enum
{
	APEX_CONFIG1_DMP_ODR_25Hz   = (0x0 ), /**< 25Hz (40ms) */
	APEX_CONFIG1_DMP_ODR_50Hz   = (0x2 ), /**< 50Hz (20ms) */
	APEX_CONFIG1_DMP_ODR_100Hz  = (0x3 ), /**< 100Hz (10ms) */
	APEX_CONFIG1_DMP_ODR_400Hz  = (0x1 ), /**< 400Hz (2.5ms) */
} APEX_CONFIG1_DMP_ODR_t;

/* DMP_FF_EN */
typedef enum
{
	APEX_CONFIG1_DMP_FF_DIS = (0x00 ),
	APEX_CONFIG1_DMP_FF_EN  = (0x01 ),
} APEX_CONFIG1_DMP_FF_EN_t;

/* DMP_SMD_EN */
typedef enum
{
	APEX_CONFIG1_DMP_SMD_DIS = (0x00 ),
	APEX_CONFIG1_DMP_SMD_EN  = (0x01 )
} APEX_CONFIG1_DMP_SMD_EN_t;


/*
 * WOM_CONFIG
 * Register Name: WOM_CONFIG
 */

/* WOM_INT_DUR */
typedef enum
{
	WOM_CONFIG_WOM_INT_DUR_1_SMPL = (0x00 ),
	WOM_CONFIG_WOM_INT_DUR_2_SMPL = (0x01 ),
	WOM_CONFIG_WOM_INT_DUR_3_SMPL = (0x02 ),
	WOM_CONFIG_WOM_INT_DUR_4_SMPL = (0x03 ),
} WOM_CONFIG_WOM_INT_DUR_t;

/* WOM_INT_MODE */
typedef enum
{
	WOM_CONFIG_WOM_INT_MODE_ANDED = (0x01 ),
	WOM_CONFIG_WOM_INT_MODE_ORED  = (0x00 ),
} WOM_CONFIG_WOM_INT_MODE_t;

/* WOM_MODE */
typedef enum
{
	WOM_CONFIG_WOM_MODE_CMP_PREV = (0x01 ),
	WOM_CONFIG_WOM_MODE_CMP_INIT = (0x00 ),
} WOM_CONFIG_WOM_MODE_t;

/* WOM_ENABLE */
typedef enum
{
	WOM_CONFIG_WOM_EN_ENABLE  = (0x01 ),
	WOM_CONFIG_WOM_EN_DISABLE = (0x00 ),
} WOM_CONFIG_WOM_EN_t;


/* ----------------------------------------------------------------------------
 * Register bank 1 (MREG1)
 * ---------------------------------------------------------------------------- */
/*
 * TMST_CONFIG1
 * Register Name: TMST_CONFIG1
 */

/* TMST_TO_REGS */
typedef enum
{
	TMST_CONFIG1_TMST_ON_SREG_EN   = (0x1 ),
	TMST_CONFIG1_TMST_ON_SREG_DIS  = (0x0 ),
} TMST_CONFIG1_TMST_ON_SREG_EN_t;

/* TMST_RES */
typedef enum
{
	TMST_CONFIG1_RESOL_16us = (0x01 ),
	TMST_CONFIG1_RESOL_1us  = (0x00 ),
} TMST_CONFIG1_RESOL_t;

/* TMST_FSYNC */
typedef enum
{
	TMST_CONFIG1_TMST_FSYNC_EN  = (0x01 ),
	TMST_CONFIG1_TMST_FSYNC_DIS = (0x00 ),
} TMST_CONFIG1_TMST_FSYNC_EN_t;

/* TMST_EN */
typedef enum
{
	TMST_CONFIG1_TMST_EN  = 0x01,
	TMST_CONFIG1_TMST_DIS = 0x00,
} TMST_CONFIG1_TMST_EN_t;


/*
 * FIFO_CONFIG5
 * Register Name: FIFO_CONFIG5
 */
/* FIFO_WM_GT_TH */
typedef enum
{
	FIFO_CONFIG5_WM_GT_TH_EN  = (0x1 ),
	FIFO_CONFIG5_WM_GT_TH_DIS = (0x0 ),
} FIFO_CONFIG5_WM_GT_t;

/* FIFO_HIRES_EN */
typedef enum
{
	FIFO_CONFIG5_HIRES_EN  = (0x1 ),
	FIFO_CONFIG5_HIRES_DIS = (0x0 ),
} FIFO_CONFIG5_HIRES_t;

/* FIFO_TMST_FSYNC_EN */
typedef enum
{
	FIFO_CONFIG5_TMST_FSYNC_EN  = (0x1 ),
	FIFO_CONFIG5_TMST_FSYNC_DIS = (0x0 ),
} FIFO_CONFIG5_TMST_FSYNC_t;

/* FIFO_GYRO_EN */
typedef enum
{
	FIFO_CONFIG5_GYRO_EN  = (0x1 ),
	FIFO_CONFIG5_GYRO_DIS = (0x0 ),
} FIFO_CONFIG5_GYRO_t;

/* FIFO_ACCEL_EN*/
typedef enum
{
	FIFO_CONFIG5_ACCEL_EN  = 0x01,
	FIFO_CONFIG5_ACCEL_DIS = 0x00,
} FIFO_CONFIG5_ACCEL_t;

/*
 * FIFO_CONFIG6
 */
 
/*
 * rcosc_req_on_fifo_ths_dis 
 * 0: when FIFO is operating in ALP mode and the watermark interrupt is enabled, FIFO wakes up the system oscillator as soon as the watermark level is reached. The oscillator remains enabled until a Host FIFO read operation happens. As side effect un extra power consumption could be seen in LP mode when the RCOSC is expected to be off.
 * 1: RCOSC is not automatically requested by the FIFO/INT circuit when the WM interrupt is triggered. As side effect the host can receive invalid packets until RCOSC is off.
 */
typedef enum
{
	FIFO_CONFIG6_RCOSC_REQ_ON_FIFO_THS_EN  = (0x00),
	FIFO_CONFIG6_RCOSC_REQ_ON_FIFO_THS_DIS = (0x01),
} FIFO_CONFIG6_RCOSC_REQ_t;


/*
 * APEX_CONFIG2
 * Register Name: APEX_CONFIG2
*/

/* DMP_POWER_SAVE_TIME_SEL */
typedef enum
{
	APEX_CONFIG2_DMP_POWER_SAVE_TIME_SEL_0S  = 0x0,
	APEX_CONFIG2_DMP_POWER_SAVE_TIME_SEL_4S  = 0x1,
	APEX_CONFIG2_DMP_POWER_SAVE_TIME_SEL_8S  = 0x2,
	APEX_CONFIG2_DMP_POWER_SAVE_TIME_SEL_12S = 0x3,
	APEX_CONFIG2_DMP_POWER_SAVE_TIME_SEL_16S = 0x4,
	APEX_CONFIG2_DMP_POWER_SAVE_TIME_SEL_20S = 0x5,
	APEX_CONFIG2_DMP_POWER_SAVE_TIME_SEL_24S = 0x6,
	APEX_CONFIG2_DMP_POWER_SAVE_TIME_SEL_28S = 0x7,
	APEX_CONFIG2_DMP_POWER_SAVE_TIME_SEL_32S = 0x8,
	APEX_CONFIG2_DMP_POWER_SAVE_TIME_SEL_36S = 0x9,
	APEX_CONFIG2_DMP_POWER_SAVE_TIME_SEL_40S = 0xA,
	APEX_CONFIG2_DMP_POWER_SAVE_TIME_SEL_44S = 0xB,
	APEX_CONFIG2_DMP_POWER_SAVE_TIME_SEL_48S = 0xC,
	APEX_CONFIG2_DMP_POWER_SAVE_TIME_SEL_52S = 0xD,
	APEX_CONFIG2_DMP_POWER_SAVE_TIME_SEL_56S = 0xE,
	APEX_CONFIG2_DMP_POWER_SAVE_TIME_SEL_60S = 0xF

} APEX_CONFIG2_DMP_POWER_SAVE_TIME_t;

/* LOW_ENERGY_AMP_TH_SEL */
typedef enum
{
	APEX_CONFIG2_LOW_ENERGY_AMP_TH_SEL_1006632MG = (0 ),
	APEX_CONFIG2_LOW_ENERGY_AMP_TH_SEL_1174405MG = (1 ),
	APEX_CONFIG2_LOW_ENERGY_AMP_TH_SEL_1342177MG = (2 ),
	APEX_CONFIG2_LOW_ENERGY_AMP_TH_SEL_1509949MG = (3 ),
	APEX_CONFIG2_LOW_ENERGY_AMP_TH_SEL_1677721MG = (4 ),
	APEX_CONFIG2_LOW_ENERGY_AMP_TH_SEL_1845493MG = (5 ),
	APEX_CONFIG2_LOW_ENERGY_AMP_TH_SEL_2013265MG = (6 ),
	APEX_CONFIG2_LOW_ENERGY_AMP_TH_SEL_2181038MG = (7 ),
	APEX_CONFIG2_LOW_ENERGY_AMP_TH_SEL_2348810MG = (8 ),
	APEX_CONFIG2_LOW_ENERGY_AMP_TH_SEL_2516582MG = (9 ),
	APEX_CONFIG2_LOW_ENERGY_AMP_TH_SEL_2684354MG = (10 ),
	APEX_CONFIG2_LOW_ENERGY_AMP_TH_SEL_2852126MG = (11 ),
	APEX_CONFIG2_LOW_ENERGY_AMP_TH_SEL_3019898MG = (12 ),
	APEX_CONFIG2_LOW_ENERGY_AMP_TH_SEL_3187671MG = (13 ),
	APEX_CONFIG2_LOW_ENERGY_AMP_TH_SEL_3355443MG = (14 ),
	APEX_CONFIG2_LOW_ENERGY_AMP_TH_SEL_3523215MG = (15 )
} APEX_CONFIG2_LOW_ENERGY_AMP_TH_t;

/*
 * APEX_CONFIG3
 * Register Name: APEX_CONFIG3
*/
/* PEDO_AMP_TH_SEL */
typedef enum
{
	APEX_CONFIG3_PEDO_AMP_TH_1006632_MG = (0  ),
	APEX_CONFIG3_PEDO_AMP_TH_1140850_MG = (1  ),
	APEX_CONFIG3_PEDO_AMP_TH_1275068_MG = (2  ),
	APEX_CONFIG3_PEDO_AMP_TH_1409286_MG = (3  ),
	APEX_CONFIG3_PEDO_AMP_TH_1543503_MG = (4  ),
	APEX_CONFIG3_PEDO_AMP_TH_1677721_MG = (5  ),
	APEX_CONFIG3_PEDO_AMP_TH_1811939_MG = (6  ),
	APEX_CONFIG3_PEDO_AMP_TH_1946157_MG = (7  ),
	APEX_CONFIG3_PEDO_AMP_TH_2080374_MG = (8  ),
	APEX_CONFIG3_PEDO_AMP_TH_2214592_MG = (9  ),
	APEX_CONFIG3_PEDO_AMP_TH_2348810_MG = (10 ),
	APEX_CONFIG3_PEDO_AMP_TH_2483027_MG = (11 ),
	APEX_CONFIG3_PEDO_AMP_TH_2617245_MG = (12 ),
	APEX_CONFIG3_PEDO_AMP_TH_2751463_MG = (13 ),
	APEX_CONFIG3_PEDO_AMP_TH_2885681_MG = (14 ),
	APEX_CONFIG3_PEDO_AMP_TH_3019898_MG = (15 )
} APEX_CONFIG3_PEDO_AMP_TH_t;

/*
 * APEX_CONFIG4
 * Register Name: APEX_CONFIG4
*/

/* PEDO_SB_TIMER_TH_SEL */
typedef enum
{
	APEX_CONFIG4_PEDO_SB_TIMER_TH_50_SAMPLES  = (0  ),
	APEX_CONFIG4_PEDO_SB_TIMER_TH_75_SAMPLES  = (1  ),
	APEX_CONFIG4_PEDO_SB_TIMER_TH_100_SAMPLES = (2  ),
	APEX_CONFIG4_PEDO_SB_TIMER_TH_125_SAMPLES = (3  ),
	APEX_CONFIG4_PEDO_SB_TIMER_TH_150_SAMPLES = (4  ),
	APEX_CONFIG4_PEDO_SB_TIMER_TH_175_SAMPLES = (5  ),
	APEX_CONFIG4_PEDO_SB_TIMER_TH_200_SAMPLES = (6  ),
	APEX_CONFIG4_PEDO_SB_TIMER_TH_225_SAMPLES = (7  )
} APEX_CONFIG4_PEDO_SB_TIMER_TH_t;


/* PEDO_HI_ENRGY_TH_SEL */
typedef enum
{
	APEX_CONFIG4_PEDO_HI_ENRGY_TH_90  = (0 ),
	APEX_CONFIG4_PEDO_HI_ENRGY_TH_107 = (1 ),
	APEX_CONFIG4_PEDO_HI_ENRGY_TH_136 = (2 ),
	APEX_CONFIG4_PEDO_HI_ENRGY_TH_159 = (3 )
} APEX_CONFIG4_PEDO_HI_ENRGY_TH_t;


/*
 * APEX_CONFIG5
 * Register Name: APEX_CONFIG5
*/
/* TILT_WAIT_TIME_SEL */
typedef enum
{
	APEX_CONFIG5_TILT_WAIT_TIME_0S = (0  ),
	APEX_CONFIG5_TILT_WAIT_TIME_2S = (1  ),
	APEX_CONFIG5_TILT_WAIT_TIME_4S = (2  ),
	APEX_CONFIG5_TILT_WAIT_TIME_6S = (3  )
} APEX_CONFIG5_TILT_WAIT_TIME_t;

/* LOWG_PEAK_TH_HYST_SEL */
typedef enum
{
  APEX_CONFIG5_LOWG_PEAK_TH_HYST_31MG  = (0  ),
  APEX_CONFIG5_LOWG_PEAK_TH_HYST_63MG  = (1  ),
  APEX_CONFIG5_LOWG_PEAK_TH_HYST_94MG  = (2  ),
  APEX_CONFIG5_LOWG_PEAK_TH_HYST_125MG = (3  ),
  APEX_CONFIG5_LOWG_PEAK_TH_HYST_156MG = (4  ),
  APEX_CONFIG5_LOWG_PEAK_TH_HYST_188MG = (5  ),
  APEX_CONFIG5_LOWG_PEAK_TH_HYST_219MG = (6  ),
  APEX_CONFIG5_LOWG_PEAK_TH_HYST_250MG = (7  )
} APEX_CONFIG5_LOWG_PEAK_TH_HYST_t;

/* HIGHG_PEAK_TH_HYST_SEL */
typedef enum
{
  APEX_CONFIG5_HIGHG_PEAK_TH_HYST_31MG  = (0  ),
  APEX_CONFIG5_HIGHG_PEAK_TH_HYST_63MG  = (1  ),
  APEX_CONFIG5_HIGHG_PEAK_TH_HYST_94MG  = (2  ),
  APEX_CONFIG5_HIGHG_PEAK_TH_HYST_125MG = (3  ),
  APEX_CONFIG5_HIGHG_PEAK_TH_HYST_156MG = (4  ),
  APEX_CONFIG5_HIGHG_PEAK_TH_HYST_188MG = (5  ),
  APEX_CONFIG5_HIGHG_PEAK_TH_HYST_219MG = (6  ),
  APEX_CONFIG5_HIGHG_PEAK_TH_HYST_250MG = (7  )
} APEX_CONFIG5_HIGHG_PEAK_TH_HYST_t;

/*
 * APEX_CONFIG9
 * Register Name: APEX_CONFIG9
*/

/* FF_DEBOUNCE_DURATION_SEL */
typedef enum
{
	APEX_CONFIG9_FF_DEBOUNCE_DURATION_0_MS = (0  ),
	APEX_CONFIG9_FF_DEBOUNCE_DURATION_1250_MS = (1  ),
	APEX_CONFIG9_FF_DEBOUNCE_DURATION_1375_MS = (2  ),
	APEX_CONFIG9_FF_DEBOUNCE_DURATION_1500_MS = (3  ),
	APEX_CONFIG9_FF_DEBOUNCE_DURATION_1625_MS = (4  ),
	APEX_CONFIG9_FF_DEBOUNCE_DURATION_1750_MS = (5  ),
	APEX_CONFIG9_FF_DEBOUNCE_DURATION_1875_MS = (6  ),
	APEX_CONFIG9_FF_DEBOUNCE_DURATION_2000_MS = (7  ),
	APEX_CONFIG9_FF_DEBOUNCE_DURATION_2125_MS = (8  ),
	APEX_CONFIG9_FF_DEBOUNCE_DURATION_2250_MS = (9  ),
	APEX_CONFIG9_FF_DEBOUNCE_DURATION_2375_MS = (10 ),
	APEX_CONFIG9_FF_DEBOUNCE_DURATION_2500_MS = (11 ),
	APEX_CONFIG9_FF_DEBOUNCE_DURATION_2625_MS = (12 ),
	APEX_CONFIG9_FF_DEBOUNCE_DURATION_2750_MS = (13 ),
	APEX_CONFIG9_FF_DEBOUNCE_DURATION_2875_MS = (14 ),
	APEX_CONFIG9_FF_DEBOUNCE_DURATION_3000_MS = (15 )
} APEX_CONFIG9_FF_DEBOUNCE_DURATION_t;

/* SMD_SENSITIVITY_SEL */
typedef enum
{
	APEX_CONFIG9_SMD_SENSITIVITY_0 = (0 ),
	APEX_CONFIG9_SMD_SENSITIVITY_1 = (1 ),
	APEX_CONFIG9_SMD_SENSITIVITY_2 = (2 ),
	APEX_CONFIG9_SMD_SENSITIVITY_3 = (3 ),
	APEX_CONFIG9_SMD_SENSITIVITY_4 = (4 )
} APEX_CONFIG9_SMD_SENSITIVITY_t;

/* SMD_SENSITIVITY_MODE */
typedef enum
{
	APEX_CONFIG9_SENSITIVITY_MODE_NORMAL    = (0 ),
	APEX_CONFIG9_SENSITIVITY_MODE_SLOW_WALK = (1 )
} APEX_CONFIG9_SENSITIVITY_MODE_t;


/*
 * APEX_CONFIG10_MREG
 * Register Name: APEX_CONFIG10
*/
typedef enum
{
	APEX_CONFIG10_LOWG_PEAK_TH_31MG    = (0x00 ),
	APEX_CONFIG10_LOWG_PEAK_TH_63MG    = (0x01 ),
	APEX_CONFIG10_LOWG_PEAK_TH_94MG    = (0x02 ),
	APEX_CONFIG10_LOWG_PEAK_TH_125MG   = (0x03 ),
	APEX_CONFIG10_LOWG_PEAK_TH_156MG   = (0x04 ),
	APEX_CONFIG10_LOWG_PEAK_TH_188MG   = (0x05 ),
	APEX_CONFIG10_LOWG_PEAK_TH_219MG   = (0x06 ),
	APEX_CONFIG10_LOWG_PEAK_TH_250MG   = (0x07 ),
	APEX_CONFIG10_LOWG_PEAK_TH_281MG   = (0x08 ),
	APEX_CONFIG10_LOWG_PEAK_TH_313MG   = (0x09 ),
	APEX_CONFIG10_LOWG_PEAK_TH_344MG   = (0x0A ),
	APEX_CONFIG10_LOWG_PEAK_TH_375MG   = (0x0B ),
	APEX_CONFIG10_LOWG_PEAK_TH_406MG   = (0x0C ),
	APEX_CONFIG10_LOWG_PEAK_TH_438MG   = (0x0D ),
	APEX_CONFIG10_LOWG_PEAK_TH_469MG   = (0x0E ),
	APEX_CONFIG10_LOWG_PEAK_TH_500MG   = (0x0F ),
	APEX_CONFIG10_LOWG_PEAK_TH_531MG   = (0x10 ),
	APEX_CONFIG10_LOWG_PEAK_TH_563MG   = (0x11 ),
	APEX_CONFIG10_LOWG_PEAK_TH_594MG   = (0x12 ),
	APEX_CONFIG10_LOWG_PEAK_TH_625MG   = (0x13 ),
	APEX_CONFIG10_LOWG_PEAK_TH_656MG   = (0x14 ),
	APEX_CONFIG10_LOWG_PEAK_TH_688MG   = (0x15 ),
	APEX_CONFIG10_LOWG_PEAK_TH_719MG   = (0x16 ),
	APEX_CONFIG10_LOWG_PEAK_TH_750MG   = (0x17 ),
	APEX_CONFIG10_LOWG_PEAK_TH_781MG   = (0x18 ),
	APEX_CONFIG10_LOWG_PEAK_TH_813MG   = (0x19 ),
	APEX_CONFIG10_LOWG_PEAK_TH_844MG   = (0x1A ),
	APEX_CONFIG10_LOWG_PEAK_TH_875MG   = (0x1B ),
	APEX_CONFIG10_LOWG_PEAK_TH_906MG   = (0x1C ),
	APEX_CONFIG10_LOWG_PEAK_TH_938MG   = (0x1D ),
	APEX_CONFIG10_LOWG_PEAK_TH_969MG   = (0x1E ),
	APEX_CONFIG10_LOWG_PEAK_TH_1000MG  = (0x1F )
} APEX_CONFIG10_LOWG_PEAK_TH_t;

typedef enum
{
	APEX_CONFIG10_LOWG_TIME_TH_1_SAMPLE  = (0x00 ),
	APEX_CONFIG10_LOWG_TIME_TH_2_SAMPLES = (0x01 ),
	APEX_CONFIG10_LOWG_TIME_TH_3_SAMPLES = (0x02 ),
	APEX_CONFIG10_LOWG_TIME_TH_4_SAMPLES = (0x03 ),
	APEX_CONFIG10_LOWG_TIME_TH_5_SAMPLES = (0x04 ),
	APEX_CONFIG10_LOWG_TIME_TH_6_SAMPLES = (0x05 ),
	APEX_CONFIG10_LOWG_TIME_TH_7_SAMPLES = (0x06 ),
	APEX_CONFIG10_LOWG_TIME_TH_8_SAMPLES = (0x07 )
} APEX_CONFIG10_LOWG_TIME_TH_SAMPLES_t;

/*
 * APEX_CONFIG11_MREG
 * Register Name: APEX_CONFIG11
*/
typedef enum
{
	APEX_CONFIG11_HIGHG_PEAK_TH_250MG   = (0x00 ),
	APEX_CONFIG11_HIGHG_PEAK_TH_500MG   = (0x01 ),
	APEX_CONFIG11_HIGHG_PEAK_TH_750MG   = (0x02 ),
	APEX_CONFIG11_HIGHG_PEAK_TH_1000MG  = (0x03 ),
	APEX_CONFIG11_HIGHG_PEAK_TH_1250MG  = (0x04 ),
	APEX_CONFIG11_HIGHG_PEAK_TH_1500MG  = (0x05 ),
	APEX_CONFIG11_HIGHG_PEAK_TH_1750MG  = (0x06 ),
	APEX_CONFIG11_HIGHG_PEAK_TH_2000MG  = (0x07 ),
	APEX_CONFIG11_HIGHG_PEAK_TH_2250MG  = (0x08 ),
	APEX_CONFIG11_HIGHG_PEAK_TH_2500MG  = (0x09 ),
	APEX_CONFIG11_HIGHG_PEAK_TH_2750MG  = (0x0A ),
	APEX_CONFIG11_HIGHG_PEAK_TH_3000MG  = (0x0B ),
	APEX_CONFIG11_HIGHG_PEAK_TH_3250MG  = (0x0C ),
	APEX_CONFIG11_HIGHG_PEAK_TH_3500MG  = (0x0D ),
	APEX_CONFIG11_HIGHG_PEAK_TH_3750MG  = (0x0E ),
	APEX_CONFIG11_HIGHG_PEAK_TH_4000MG  = (0x0F ),
	APEX_CONFIG11_HIGHG_PEAK_TH_4250MG  = (0x10 ),
	APEX_CONFIG11_HIGHG_PEAK_TH_4500MG  = (0x11 ),
	APEX_CONFIG11_HIGHG_PEAK_TH_4750MG  = (0x12 ),
	APEX_CONFIG11_HIGHG_PEAK_TH_5000MG  = (0x13 ),
	APEX_CONFIG11_HIGHG_PEAK_TH_5250MG  = (0x14 ),
	APEX_CONFIG11_HIGHG_PEAK_TH_5500MG  = (0x15 ),
	APEX_CONFIG11_HIGHG_PEAK_TH_5750MG  = (0x16 ),
	APEX_CONFIG11_HIGHG_PEAK_TH_6000MG  = (0x17 ),
	APEX_CONFIG11_HIGHG_PEAK_TH_6250MG  = (0x18 ),
	APEX_CONFIG11_HIGHG_PEAK_TH_6500MG  = (0x19 ),
	APEX_CONFIG11_HIGHG_PEAK_TH_6750MG  = (0x1A ),
	APEX_CONFIG11_HIGHG_PEAK_TH_7000MG  = (0x1B ),
	APEX_CONFIG11_HIGHG_PEAK_TH_7250MG  = (0x1C ),
	APEX_CONFIG11_HIGHG_PEAK_TH_7500MG  = (0x1D ),
	APEX_CONFIG11_HIGHG_PEAK_TH_7750MG  = (0x1E ),
	APEX_CONFIG11_HIGHG_PEAK_TH_8000MG  = (0x1F )
} APEX_CONFIG11_HIGHG_PEAK_TH_t;

typedef enum
{
	APEX_CONFIG11_HIGHG_TIME_TH_1_SAMPLE  = (0x00 ),
	APEX_CONFIG11_HIGHG_TIME_TH_2_SAMPLES = (0x01 ),
	APEX_CONFIG11_HIGHG_TIME_TH_3_SAMPLES = (0x02 ),
	APEX_CONFIG11_HIGHG_TIME_TH_4_SAMPLES = (0x03 ),
	APEX_CONFIG11_HIGHG_TIME_TH_5_SAMPLES = (0x04 ),
	APEX_CONFIG11_HIGHG_TIME_TH_6_SAMPLES = (0x05 ),
	APEX_CONFIG11_HIGHG_TIME_TH_7_SAMPLES = (0x06 ),
	APEX_CONFIG11_HIGHG_TIME_TH_8_SAMPLES = (0x07 )
} APEX_CONFIG11_HIGHG_TIME_TH_SAMPLES_t;

/*
 * APEX_CONFIG12_MREG
 * Register Name: APEX_CONFIG12
*/
/* FF_MAX_DURATION_SEL */
typedef enum
{
	APEX_CONFIG12_FF_MAX_DURATION_102_CM = (0  ),
	APEX_CONFIG12_FF_MAX_DURATION_120_CM = (1  ),
	APEX_CONFIG12_FF_MAX_DURATION_139_CM = (2  ),
	APEX_CONFIG12_FF_MAX_DURATION_159_CM = (3  ),
	APEX_CONFIG12_FF_MAX_DURATION_181_CM = (4  ),
	APEX_CONFIG12_FF_MAX_DURATION_204_CM = (5  ),
	APEX_CONFIG12_FF_MAX_DURATION_228_CM = (6  ),
	APEX_CONFIG12_FF_MAX_DURATION_254_CM = (7  ),
	APEX_CONFIG12_FF_MAX_DURATION_281_CM = (8  ),
	APEX_CONFIG12_FF_MAX_DURATION_310_CM = (9  ),
	APEX_CONFIG12_FF_MAX_DURATION_339_CM = (10 ),
	APEX_CONFIG12_FF_MAX_DURATION_371_CM = (11 ),
	APEX_CONFIG12_FF_MAX_DURATION_403_CM = (12 ),
	APEX_CONFIG12_FF_MAX_DURATION_438_CM = (13 ),
	APEX_CONFIG12_FF_MAX_DURATION_473_CM = (14 ),
	APEX_CONFIG12_FF_MAX_DURATION_510_CM = (15 )
} APEX_CONFIG12_FF_MAX_DURATION_t;

/* FF_MIN_DURATION_SEL */
typedef enum
{
	APEX_CONFIG12_FF_MIN_DURATION_10_CM = (0  ),
	APEX_CONFIG12_FF_MIN_DURATION_12_CM = (1  ),
	APEX_CONFIG12_FF_MIN_DURATION_13_CM = (2  ),
	APEX_CONFIG12_FF_MIN_DURATION_16_CM = (3  ),
	APEX_CONFIG12_FF_MIN_DURATION_18_CM = (4  ),
	APEX_CONFIG12_FF_MIN_DURATION_20_CM = (5  ),
	APEX_CONFIG12_FF_MIN_DURATION_23_CM = (6  ),
	APEX_CONFIG12_FF_MIN_DURATION_25_CM = (7  ),
	APEX_CONFIG12_FF_MIN_DURATION_28_CM = (8  ),
	APEX_CONFIG12_FF_MIN_DURATION_31_CM = (9  ),
	APEX_CONFIG12_FF_MIN_DURATION_34_CM = (10 ),
	APEX_CONFIG12_FF_MIN_DURATION_38_CM = (11 ),
	APEX_CONFIG12_FF_MIN_DURATION_41_CM = (12 ),
	APEX_CONFIG12_FF_MIN_DURATION_45_CM = (13 ),
	APEX_CONFIG12_FF_MIN_DURATION_48_CM = (14 ),
	APEX_CONFIG12_FF_MIN_DURATION_52_CM = (15 )
} APEX_CONFIG12_FF_MIN_DURATION_t;


/*
 * ST_CONFIG_MREG
 * Register Name : ST_CONFIG
 */

/*
 * st_number_sample 
 * This bit selects the number of sensor samples that should be used to process self-test.
 * 0: 16 samples.
 * 1: 200 samples.
 */
typedef enum {
	ST_CONFIG_ST_NUMBER_SAMPLE_16    = (0 ),
	ST_CONFIG_ST_NUMBER_SAMPLE_200   = (1 ),
} ST_CONFIG_ST_NUMBER_SAMPLE_t;

/*
 * accel_st_lim 
 * These bits control the tolerated ratio between self-test processed values and reference (fused) ones for accelerometer. 
 * 0 : 5%
 * 1: 10%
 * 2: 15%
 * 3: 20%
 * 4: 25%
 * 5: 30%
 * 6: 40%
 * 7: 50%
 we only support 50 % value now 
 */

#if 0
typedef enum {
	ST_CONFIG_ACCEL_ST_LIM_5  = (0 ),
	ST_CONFIG_ACCEL_ST_LIM_10 = (1 ),
	ST_CONFIG_ACCEL_ST_LIM_15 = (2 ),
	ST_CONFIG_ACCEL_ST_LIM_20 = (3 ),
	ST_CONFIG_ACCEL_ST_LIM_25 = (4 ),
	ST_CONFIG_ACCEL_ST_LIM_30 = (5 ),
	ST_CONFIG_ACCEL_ST_LIM_40 = (6 ),
	ST_CONFIG_ACCEL_ST_LIM_50 = (7 )
} ST_CONFIG_ACCEL_ST_LIM_t;
#else
#define ST_CONFIG_ACCEL_ST_LIM_50   7
#endif

/*
 * gyro_st_lim 
 * These bits control the tolerated ratio between self-test processed values and reference (fused) ones for gyro. 
 * 0 : 5%
 * 1: 10%
 * 2: 15%
 * 3: 20%
 * 4: 25%
 * 5: 30%
 * 6: 40%
 * 7: 50%
 we only support 50 % value now 
 */

#if 0
typedef enum {
	ST_CONFIG_GYRO_ST_LIM_5  = (0 ),
	ST_CONFIG_GYRO_ST_LIM_10 = (1 ),
	ST_CONFIG_GYRO_ST_LIM_15 = (2 ),
	ST_CONFIG_GYRO_ST_LIM_20 = (3 ),
	ST_CONFIG_GYRO_ST_LIM_25 = (4 ),
	ST_CONFIG_GYRO_ST_LIM_30 = (5 ),
	ST_CONFIG_GYRO_ST_LIM_40 = (6 ),
	ST_CONFIG_GYRO_ST_LIM_50 = (7 )
} ST_CONFIG_GYRO_ST_LIM_t;
#else
#define ST_CONFIG_GYRO_ST_LIM_50    7
#endif

/*
 * SELFTEST
 * Register Name : SELFTEST
 */
/*
 * gyro_st_en 
 * 1: enably gyro self test operation. Host needs to program this bit to 0 to move chip out of self test mode. If host programs this bit to 0 while st_busy = 1 and st_done =0, the current running self-test operation is terminated by host.
 */
 typedef enum {
	SELFTEST_GYRO_ST_EN_EN  = (1 ),
	SELFTEST_GYRO_ST_EN_DIS = (0 ),
} SELFTEST_GYRO_ST_EN_t;

/*
 * accel_st_en 
 * 1: enably accel self test operation. Host needs to program this bit to 0 to move chip out of self test mode. If host programs this bit to 0 while st_busy = 1 and st_done =0, the current running self-test operation is terminated by host.
 */
 typedef enum {
	SELFTEST_ACCEL_ST_EN_EN  = (1 ),
	SELFTEST_ACCEL_ST_EN_DIS = (0 ),
} SELFTEST_ACCEL_ST_EN_t;


/*
 * ST_COPY_EN
 * (OTP_CONFIG_MREG_TOP1, otp_copy_mode )
 * Specify OTP copy mode.
 * 0: Reserved
 * 1: copy OTP blk 1-n to SRAM, except the self-test related data.
 * 2: copy security authentication keys. (Reserved for customer).
 * 3: copy self-test data from OTP blk 1-n to SRAM.
 */
typedef enum {
	OTP_COPY_EN_TRIM    = (0x01 ),
	OTP_COPY_EN_DATA    = (0x03 ),
} ST_COPY_EN_t;


/*
 * INT_CONFIG
 * Register Name: INT_CONFIG
 */

/* INT2_DRIVE_CIRCUIT */
typedef enum
{
	INT_CONFIG_INT2_DRIVE_CIRCUIT_PP = (0x01 ),
	INT_CONFIG_INT2_DRIVE_CIRCUIT_OD = (0x00 ),
} INT_CONFIG_INT2_DRIVE_CIRCUIT_t;


/* INT2_POLARITY */
typedef enum
{
	INT_CONFIG_INT2_POLARITY_HIGH = (0x01 ),
	INT_CONFIG_INT2_POLARITY_LOW  = (0x00 ),
} INT_CONFIG_INT2_POLARITY_t;


/* INT1_DRIVE_CIRCUIT */
typedef enum
{
	INT_CONFIG_INT1_DRIVE_CIRCUIT_PP = (0x01 ),
	INT_CONFIG_INT1_DRIVE_CIRCUIT_OD = (0x00 ),
} INT_CONFIG_INT1_DRIVE_CIRCUIT_t;


/* INT1_POLARITY */
typedef enum
{
	INT_CONFIG_INT1_POLARITY_HIGH = 0x01,
	INT_CONFIG_INT1_POLARITY_LOW  = 0x00,
} INT_CONFIG_INT1_POLARITY_t;


/* INT1_POLARITY */
typedef enum
{
	INT_CONFIG_INT1_LATCH  = 0x01,
	INT_CONFIG_INT1_PULSE  = 0x00,
} INT_CONFIG_INT1_MODE_t;

/** @brief Configure Fifo usage
 */

#define    INV_IMU_FIFO_DISABLED    0              /**< Fifo is disabled and data source is sensors registers */
#define    INV_IMU_FIFO_ENABLED     1               /**< Fifo is used as data source */



/* ----------------------------------------------------------------------------
 * Register bank 2
 * MREG2
 * MREG_OTP
 * ---------------------------------------------------------------------------- */

/* TRIGGER_ST_COPY
 * OTP_CTRL7 
 * 1: Triggers self-test data copy from OTP to SRAM; ST_COPY_EN must be set to b11 (0x03) to use this bit
 */
typedef enum
{
	OTP_CTRL7_RELOAD_EN   = 0x01,
	OTP_CTRL7_RELOAD_DIS  = 0x00,
} OTP_CTRL7_RELOAD_t;

/* OTP_CTRL7_PWR_DOWN
 * 0: Power up OTP to copy from OTP to SRAM
 * 1: Power down OTP
 */
typedef enum
{
	OTP_CTRL7_PWR_DOWN   = 0x01,
	OTP_CTRL7_PWR_UP     = 0x00,
} OTP_CTRL7_PWR_DOWN_t;

/*
 * activity_class 
 * The walk/run activity classification is also an output in pedometer. This 2-bit register report the current user activity as following: 
 * 0: unknown activity as default output;
 * 1: walk
 * 2: run (the cadence is faster than a predefined threshold).
 */
#define APEX_DATA3_ACTIVITY_CLASS_POS      0x00
#define APEX_DATA3_ACTIVITY_CLASS_MASK     0x03


/* ---------------------------------------------------------------------------- */


/** @brief IMU APEX inputs parameters definition
 */
typedef struct {
	APEX_CONFIG3_PEDO_AMP_TH_t pedo_amp_th;                        /**< Peak threshold value to be considered as a valid step (mg) */
	uint8_t pedo_step_cnt_th;                                               /**< Minimum number of steps that must be detected 
	                                                                             before the pedometer step count begins incrementing */
	uint8_t pedo_step_det_th;                                               /**< Minimum number of low latency steps that must be detected 
	                                                                             before the pedometer step count begins incrementing */
	APEX_CONFIG4_PEDO_SB_TIMER_TH_t pedo_sb_timer_th;              /**< Duration of non-walk to exit the current walk mode, 
	                                                                             pedo_step_cnt_th number of steps must again be detected 
	                                                                             before step count starts to increase */
	APEX_CONFIG4_PEDO_HI_ENRGY_TH_t pedo_hi_enrgy_th;              /**< Threshold to improve run detection if not steps are counted while running */
 
	APEX_CONFIG5_TILT_WAIT_TIME_t tilt_wait_time;                  /**< Number of accelerometer samples to wait before triggering tilt event */
 
	APEX_CONFIG2_DMP_POWER_SAVE_TIME_t power_save_time;            /**< The time after which DMP goes in power save mode according to the DMP ODR configured */
    
	APEX_CONFIG9_SENSITIVITY_MODE_t sensitivity_mode;             /**< Sensitivity mode Normal(0) or Slow walk(1). The Slow walk mode improve 
	                                                                             the slow walk detection (<1Hz) but in return the number of false detection 
	                                                                             might be increase. */
	APEX_CONFIG2_LOW_ENERGY_AMP_TH_t low_energy_amp_th;            /**< Peak threshold value to be considered as a valid step (mg) in Slow walk mode */
	
	APEX_CONFIG9_SMD_SENSITIVITY_t smd_sensitivity;                /**< SMD algorithm resilience to false detection in rejection use case.
	                                                                             Note that a higher value will reject more transport situation */
	
	APEX_CONFIG9_FF_DEBOUNCE_DURATION_t ff_debounce_duration;      /**< Duration(us) during which LowG and HighG events are not taken into account after an HighG event.
	                                                                             The goal is to avoid detecting bounces as free falls */
	
	APEX_CONFIG12_FF_MAX_DURATION_t ff_max_duration_cm;            /**< Distance (cm) max crossed by the device after a LowG event, below which the detection of an HighG event triggers a freefall interrupt */
	APEX_CONFIG12_FF_MIN_DURATION_t ff_min_duration_cm;            /**< Distance (cm) min crossed by the device after a LowG event, above which the detection of an HighG event triggers a freefall interrupt */
	
	APEX_CONFIG10_LOWG_PEAK_TH_t lowg_peak_th;                     /**< Absolute low peak threshold (mg) to detect when it falls below on any of the accelerometer axis */
	APEX_CONFIG5_LOWG_PEAK_TH_HYST_t lowg_peak_hyst;               /**< Hysteresis threshold (mg) added to the threshold after the initial threshold is met */
	APEX_CONFIG10_LOWG_TIME_TH_SAMPLES_t lowg_samples_th;          /**< Time in number of samples to stay below the threshold before triggering the event (samples) */
	
	APEX_CONFIG11_HIGHG_PEAK_TH_t highg_peak_th;                   /**< Absolute high peak threshold to detect when it goes above on any of the accelerometer axis */
	APEX_CONFIG5_HIGHG_PEAK_TH_HYST_t highg_peak_hyst;             /**< Hysteresis threshold (mg) substracted  to the threshold after the initial threshold is met */
	APEX_CONFIG11_HIGHG_TIME_TH_SAMPLES_t highg_samples_th;        /**< Time in number of samples to stay above the threshold before triggering the event (samples) */
	
} inv_imu_apex_parameters_t;

/** @brief APEX pedometer outputs
 */
typedef struct inv_imu_apex_step_activity {
	uint16_t step_cnt;      /**< Number of steps taken */
	uint8_t step_cadence;   /**< Walk/run cadency in number of samples. 
	                             Format is u6.2. E.g, At 50Hz and 2Hz walk frequency, if the cadency is 25 samples. The register will output 100. */
	uint8_t activity_class; /**< Detected activity unknown (0), walk (1) or run (2) */
} inv_imu_apex_step_activity_t;


/** @brief Common error code definition
 */

enum inv_error
{
	INV_ERROR_SUCCESS      = 0,   /**< no error */
	INV_ERROR              = -1,  /**< unspecified error */
	INV_ERROR_NIMPL        = -2,  /**< function not implemented for given
	                                   arguments */
	INV_ERROR_TRANSPORT    = -3,  /**< error occured at transport level */
	INV_ERROR_TIMEOUT      = -4,  /**< action did not complete in the expected
	                                   time window */
	INV_ERROR_SIZE         = -5,  /**< size/length of given arguments is not
	                                   suitable to complete requested action */
	INV_ERROR_OS           = -6,  /**< error related to OS */
	INV_ERROR_IO           = -7,  /**< error related to IO operation */
	INV_ERROR_MEM          = -9,  /**< not enough memory to complete requested
	                                   action */
	INV_ERROR_HW           = -10, /**< error at HW level */
	INV_ERROR_BAD_ARG      = -11, /**< provided arguments are not good to
	                                   perform requestion action */
	INV_ERROR_UNEXPECTED   = -12, /**< something unexpected happened */
	INV_ERROR_FILE         = -13, /**< cannot access file or unexpected format */
	INV_ERROR_PATH         = -14, /**< invalid file path */
	INV_ERROR_IMAGE_TYPE   = -15, /**< error when image type is not managed */
	INV_ERROR_WATCHDOG     = -16 /**< error when device doesn't respond 
									   to ping */
};


enum inv_log_level {
    INV_LOG_LEVEL_OFF     = 0,
    INV_LOG_LEVEL_ERROR,
    INV_LOG_LEVEL_WARNING,
    INV_LOG_LEVEL_INFO,
    INV_LOG_LEVEL_MAX
};


typedef enum {
    ACC = 0,
    GYR,
    TEMP,
    WOM,
    PEDO,
    FF,
    TS,
    NUM_OF_SENSOR,
} SensorType_t;


struct accGyroData {
    uint8_t sensType;
    int16_t x, y, z;
    int16_t temperature;
    int8_t high_res[3];
    uint64_t  timeStamp;
};

struct SensorData {
#if DATA_FORMAT_DPS_G
    float x, y, z;
#else
    int32_t x, y, z;
#endif
    uint64_t timeStamp;
};

#if DATA_FORMAT_DPS_G
typedef float chip_temperature;

#else
typedef int16_t chip_temperature;
#endif

struct accGyroDataPacket {
    uint8_t accOutSize;
    uint8_t gyroOutSize;
    uint64_t timeStamp;
    float temperature;
    struct accGyroData outBuf[MAX_RECV_PACKET*2];
    uint32_t magicNum;
};

typedef struct {
    uint8_t accDataSize;
    uint64_t timeStamp;
    struct SensorData databuff[MAX_RECV_PACKET];
}AccDataPacket;

typedef struct {
    uint8_t gyroDataSize;
    uint64_t timeStamp;
    struct SensorData databuff[MAX_RECV_PACKET];
}GyroDataPacket;

typedef struct inv_imu_selftest_output {
	int8_t accel_status;    /**< global accelerometer self-test passed */
	int8_t gyro_status;     /**< global gyroscope self-test status: st_pass (bit0), st_incomplete (bit1) */
	
} inv_imu_selftest_output_t;


int inv_imu_initialize(void);
void inv_imu_set_serif(int (*read)(void *, uint8_t, uint8_t *, uint32_t),
                            int (*write)(void *, uint8_t, const uint8_t *, uint32_t));
void inv_imu_set_delay(void (*delay_ms)(uint32_t), void (*delay_us)(uint32_t));
int inv_imu_acc_enable(void);
int inv_imu_gyro_enable(void);

int inv_imu_acc_disable(void);
int inv_imu_gyro_disable(void);

int inv_imu_set_gyro_fsr(GYRO_CONFIG0_FS_SEL_t gyro_fsr_dps);

int inv_imu_set_accel_fsr(ACCEL_CONFIG0_FS_SEL_t accel_fsr_g);


int inv_imu_acc_set_rate(float odr_hz, uint16_t packet_num,float *hw_odr);
int inv_imu_gyro_set_rate(float odr_hz, uint16_t packet_num,float *hw_odr);

int inv_imu_get_rawdata_interrupt(struct accGyroDataPacket *dataPacket);

int inv_data_handler(AccDataPacket *AccDatabuff,GyroDataPacket *GyroDatabuff,chip_temperature *chip_temper,bool polling);

/* more interface function for deep usage */
int inv_imu_enable_high_resolution_fifo(void);
int inv_imu_disable_high_resolution_fifo(void);
int inv_imu_enable_ff_register(void);
int inv_imu_disable_ff_register(void);
int inv_imu_apex_set_frequency(const APEX_CONFIG1_DMP_ODR_t frequency);
float convert_ff_duration_sample_to_cm(uint16_t ff_duration_samples);



#if 0
int inv_imu_polling_rawdata(struct accGyroDataPacket *dataPacket);
#endif

int inv_imu_run_selftest(uint8_t acc_control,uint8_t gyro_control, struct inv_imu_selftest_output * st_output);

int inv_imu_pedometer_enable(void);
int inv_imu_pedometer_disable(void);
int inv_imu_pedometer_get_event(float *cadence_step_per_sec,APEX_DATA3_ACTIVITY_CLASS_t *activity_class);

int inv_imu_wom_get_event(bool *wom_detect);
int inv_imu_wom_enable(uint8_t wom_threshold_x,uint8_t wom_threshold_y,uint8_t wom_threshold_z,uint8_t duration);
int inv_imu_wom_disable(void);

int inv_imu_freefall_enable(void);
int inv_imu_freefall_disable(void);
float inv_imu_freefall_get_event(bool *ff_detect);


void inv_imu_dumpRegs(void);


#ifdef __cplusplus
}
#endif
#endif
