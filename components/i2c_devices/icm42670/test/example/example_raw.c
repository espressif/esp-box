/*
 * ________________________________________________________________________________________________________
 * Copyright (c) 2017 InvenSense Inc. All rights reserved.
 *
 * This software, related documentation and any modifications thereto (collectively “Software? is subject
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

/* std */
#include <stdio.h>

/* board driver */
#include "common.h"
#include "uart_mngr.h"
#include "delay.h"
#include "gpio.h"
#include "timer.h"
#include "system_interface.h"

/* InvenSense utils */
#include "Invn/EmbUtils/Message.h"
#include "Invn/EmbUtils/RingBuffer.h"

#include "inv_imu_driver.h"


struct SensorData Acc_Receive_dataBuff[2048];
struct SensorData Gyro_Receive_dataBuff[2048];


AccDataPacket accData;
GyroDataPacket gyroData;
chip_temperature imu_chip_temperature;

uint32_t cur_index = 0;


/*
 * Select UART port on which INV_MSG() will be printed.
 */
#define LOG_UART_ID     INV_UART_SENSOR_CTRL

/* 
 * Define msg level 
 */
#define MSG_LEVEL       INV_MSG_LEVEL_DEBUG

/* 
 * Set of timers used throughout standalone applications 
 */
#define TIMEBASE_TIMER  INV_TIMER1
#define DELAY_TIMER     INV_TIMER2

#define SENSOR_ODR_1600HZ  1600
#define SENSOR_ODR_800HZ  800
#define SENSOR_ODR_400HZ   400
#define SENSOR_ODR_200HZ   200
#define SENSOR_ODR_100HZ   100
#define SENSOR_ODR_50HZ    50
#define SENSOR_ODR_25HZ    25
#define SENSOR_ODR_12_5HZ  12.5


#if !USE_FIFO
	/* 
	 * Buffer to keep track of the timestamp when IMU data ready interrupt fires.
	 * The buffer can contain up to 64 items in order to store one timestamp for each packet in FIFO.
	 */
	RINGBUFFER(timestamp_buffer, 64, uint64_t);
#endif

/* --------------------------------------------------------------------------------------
 *  Static variables
 * -------------------------------------------------------------------------------------- */

/* Flag set from IMU device irq handler */
static volatile int irq_from_device;


/* --------------------------------------------------------------------------------------
 *  Forward declaration
 * -------------------------------------------------------------------------------------- */
static int setup_mcu();
static void ext_interrupt_cb(void * context, unsigned int int_num);

void msg_printer(int level, const char * str, va_list ap);


/* --------------------------------------------------------------------------------------
 *  Functions definitions
 * -------------------------------------------------------------------------------------- */

/*
 * This function initializes MCU on which this software is running.
 * It configures:
 *   - a UART link used to print some messages
 *   - interrupt priority group and GPIO so that MCU can receive interrupts from IMU 
 *   - a microsecond timer requested by IMU driver to compute some delay
 *   - a microsecond timer used to get some timestamps
 *   - a serial link to communicate from MCU to IMU 
 */
static int setup_mcu()
{
	int rc = 0;

	platform_io_hal_board_init();
	
	/* configure UART */
	config_uart(LOG_UART_ID);

	/* Setup message facility to see internal traces from FW */
	PLATFORM_MSG_SETUP(MSG_LEVEL, msg_printer);

	INV_MSG(INV_MSG_LEVEL_INFO, "######################");
	INV_MSG(INV_MSG_LEVEL_INFO, "#   Example Raw AG   #");
	INV_MSG(INV_MSG_LEVEL_INFO, "######################");

	/*
	 * Configure input capture mode GPIO connected to pin EXT3-9 (pin PB03).
	 * This pin is connected to Icm406xx INT1 output and thus will receive interrupts 
	 * enabled on INT1 from the device.
	 * A callback function is also passed that will be executed each time an interrupt
	 * fires.
	*/
	platform_gpio_sensor_irq_init(INV_GPIO_INT1, ext_interrupt_cb, 0);

	/* Init timer peripheral for delay */
	rc |= platform_delay_init(DELAY_TIMER);
	
	/*
	 * Configure the timer for the timebase
	 */
	rc |= platform_timer_configure_timebase(1000000);
	platform_timer_enable(TIMEBASE_TIMER);

	inv_imu_set_serif(platform_io_hal_read_reg, platform_io_hal_write_reg);
    inv_imu_set_delay(platform_delay_ms,platform_delay_us);                  //void (*delay_ms)(uint32_t), void (*delay_us)(uint32_t))
	rc |= platform_io_hal_init();

	return rc;
}


/*
 * IMU interrupt handler.
 * Function is executed when an IMU interrupt rises on MCU.
 * This function get a timestamp and store it in the timestamp buffer.
 * Note that this function is executed in an interrupt handler and thus no protection
 * are implemented for shared variable timestamp_buffer.
 */
static void ext_interrupt_cb(void * context, unsigned int int_num)
{
	(void)context;

#if !USE_FIFO
	/* 
	 * Read timestamp from the timer dedicated to timestamping 
	 */
	uint64_t timestamp = platform_timer_get_counter(TIMEBASE_TIMER);

	if(int_num == INV_GPIO_INT1) {
		if (!RINGBUFFER_FULL(&timestamp_buffer))
			RINGBUFFER_PUSH(&timestamp_buffer, &timestamp);
	}
#endif

	irq_from_device |= TO_MASK(int_num);
}


/* --------------------------------------------------------------------------------------
 *  Functions definition
 * -------------------------------------------------------------------------------------- */

int setup_imu_device()
{
	int rc = 0;

	/* Init device */
	rc = inv_imu_initialize();
	if (rc != INV_ERROR_SUCCESS) {
		INV_MSG(INV_MSG_LEVEL_ERROR, "Failed to initialize IMU!");
		return rc;
	}

	
#if !USE_FIFO
	RINGBUFFER_CLEAR(&timestamp_buffer);
#endif

	return rc;
}


uint32_t get_imu_data()
{
    inv_data_handler(&accData,&gyroData,&imu_chip_temperature,false);
    if(accData.accDataSize != 0){
        memcpy(&Acc_Receive_dataBuff[cur_index], &accData.databuff[0], accData.accDataSize * sizeof(accData.databuff[0])); 
    	//INV_MSG(INV_MSG_LEVEL_ERROR, "acDS %u cidex %u",accData.accDataSize,cur_index);
    }
    
    if(gyroData.gyroDataSize != 0){
        memcpy(&Gyro_Receive_dataBuff[cur_index], &gyroData.databuff[0], gyroData.gyroDataSize * sizeof(gyroData.databuff[0]));  
    }

    if ((accData.accDataSize != 0) && (gyroData.gyroDataSize != 0))
         cur_index = cur_index + (uint32_t)accData.accDataSize;
    else if(accData.accDataSize != 0)
         cur_index = cur_index + (uint32_t)accData.accDataSize;
    else if(gyroData.gyroDataSize != 0)
         cur_index = cur_index + (uint32_t)gyroData.gyroDataSize;   

    return 0;
}


/* --------------------------------------------------------------------------------------
 *  Main
 * -------------------------------------------------------------------------------------- */



int main(void)
{
	int rc = 0;
	float hw_rate = 0.0;

    memset(Acc_Receive_dataBuff,0,sizeof(Acc_Receive_dataBuff));
    memset(Gyro_Receive_dataBuff,0,sizeof(Gyro_Receive_dataBuff));
	rc |= setup_mcu();
    
    
#if !USE_FIFO
    RINGBUFFER_CLEAR(&timestamp_buffer);
#endif

    /* Init device */
     rc = inv_imu_initialize();

    if (rc != INV_ERROR_SUCCESS) {
        INV_MSG(INV_MSG_LEVEL_ERROR, "Failed to initialize INV concise driver!");
        return rc;
    }
    
	INV_MSG(INV_MSG_LEVEL_INFO, "IMU device successfully initialized");

	// We may customize full scale range here.
    rc |= inv_imu_set_accel_fsr(ACCEL_CONFIG0_FS_SEL_4g);
    rc |= inv_imu_set_gyro_fsr(GYRO_CONFIG0_FS_SEL_2000dps);

    // Below settings are required to configure and enable sensors.
    rc |= inv_imu_acc_enable();
    rc |= inv_imu_acc_set_rate(SENSOR_ODR_400HZ, 2,&hw_rate);  //200Hz ODR, watermark: 2 packets.
    rc |= inv_imu_gyro_enable();
    rc |= inv_imu_gyro_set_rate(SENSOR_ODR_400HZ, 4,&hw_rate);  //50Hz ODR, watermark 4.
   
    if (rc != 0) {
        INV_LOG(SENSOR_LOG_LEVEL, "Feature enable Failed. Do nothing %d", rc);
        return rc;
    }
	INV_LOG(SENSOR_LOG_LEVEL, "HW odr setting %f ", hw_rate);

	do {
        
		/* Poll device for data */		
		if (irq_from_device & TO_MASK(INV_GPIO_INT1)) {
            
			get_imu_data();
			//INV_MSG(INV_MSG_LEVEL_ERROR, "cur idex %lu!",cur_index);
        
			inv_disable_irq();
			irq_from_device &= ~TO_MASK(INV_GPIO_INT1);
			inv_enable_irq();
             
		}
	} while(cur_index <2048);

    rc |= inv_imu_acc_disable();
    rc |= inv_imu_gyro_disable();

    #if SENSOR_LOG_TS_ONLY
    uint32_t i;
    for(i=0;i< cur_index; i++){
         INV_LOG(SENSOR_LOG_LEVEL, "ACC %d  TS  %lld ", i+1,Acc_Receive_dataBuff[i].timeStamp);  
         platform_delay_ms(5);
        }

    for(i=0;i< cur_index; i++){
         INV_LOG(SENSOR_LOG_LEVEL, "Gyro %d  TS  %lld ", i+1,Gyro_Receive_dataBuff[i].timeStamp);  
         platform_delay_ms(5);
        }
    #endif

    // following code is just repeating the previous code, just to confirm the testing could be alwasys run 
    int j = 2;
    while(1){
        
        memset(Acc_Receive_dataBuff,0,sizeof(Acc_Receive_dataBuff));
        memset(Gyro_Receive_dataBuff,0,sizeof(Gyro_Receive_dataBuff));

        memset(&accData,0,sizeof(accData));
        memset(&gyroData,0,sizeof(gyroData)); 
        
        INV_LOG(SENSOR_LOG_LEVEL, "repeating test round %d", j);
        // Below settings are required to configure and enable sensors.
        rc |= inv_imu_acc_enable();
        rc |= inv_imu_acc_set_rate(SENSOR_ODR_200HZ, 2,&hw_rate);  //200Hz ODR, watermark: 2 packets.
        rc |= inv_imu_gyro_enable();
        rc |= inv_imu_gyro_set_rate(SENSOR_ODR_200HZ, 4,&hw_rate);  //50Hz ODR, watermark 4.
   
        if (rc != 0) {
           INV_LOG(SENSOR_LOG_LEVEL, "Feature enable Failed. Do nothing %d", rc);
          return rc;
         }
	    INV_LOG(SENSOR_LOG_LEVEL, "HW odr setting %f ", hw_rate);

  
        cur_index = 0;
	    do {
        
		    /* Poll device for data */		
		    if (irq_from_device & TO_MASK(INV_GPIO_INT1)) {
            
			    get_imu_data();
			    //INV_MSG(INV_MSG_LEVEL_ERROR, "cur idex %lu!",cur_index);
        
			    inv_disable_irq();
			    irq_from_device &= ~TO_MASK(INV_GPIO_INT1);
			    inv_enable_irq();
             
		    }
	    } while(cur_index <2048);

        rc |= inv_imu_acc_disable();
        rc |= inv_imu_gyro_disable();

        #if SENSOR_LOG_TS_ONLY
      
        for(i=0;i< cur_index; i++){
         INV_LOG(SENSOR_LOG_LEVEL, "ACC %d  TS  %lld ", i+1,Acc_Receive_dataBuff[i].timeStamp);  
         platform_delay_ms(5);
        }

        for(i=0;i< cur_index; i++){
         INV_LOG(SENSOR_LOG_LEVEL, "Gyro %d  TS  %lld ", i+1,Gyro_Receive_dataBuff[i].timeStamp);  
         platform_delay_ms(5);
        }
       #endif

       j++;
     }
}

/*
 * Printer function for message facility
 */
void msg_printer(int level, const char * str, va_list ap)
{
	static char out_str[256]; /* static to limit stack usage */
	unsigned idx = 0;
	const char * s[INV_MSG_LEVEL_MAX] = {
	    "",    // INV_MSG_LEVEL_OFF
	    "[E] ", // INV_MSG_LEVEL_ERROR
	    "[W] ", // INV_MSG_LEVEL_WARNING
	    "[I] ", // INV_MSG_LEVEL_INFO
	    "[V] ", // INV_MSG_LEVEL_VERBOSE
	    "[D] ", // INV_MSG_LEVEL_DEBUG
	};
	idx += snprintf(&out_str[idx], sizeof(out_str) - idx, "%s", s[level]);
	if(idx >= (sizeof(out_str)))
		return;
	idx += vsnprintf(&out_str[idx], sizeof(out_str) - idx, str, ap);
	if(idx >= (sizeof(out_str)))
		return;
	idx += snprintf(&out_str[idx], sizeof(out_str) - idx, "\r\n");
	if(idx >= (sizeof(out_str)))
		return;

	platform_uart_mngr_puts(LOG_UART_ID, out_str, idx);
}
