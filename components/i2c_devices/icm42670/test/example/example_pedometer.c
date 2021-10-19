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

	INV_MSG(INV_MSG_LEVEL_INFO, "######################################");
	INV_MSG(INV_MSG_LEVEL_INFO, "#  #XIAN MCU Driver V1.0 PEDO EXAMPLE#");
	INV_MSG(INV_MSG_LEVEL_INFO, "######################################");

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


/* --------------------------------------------------------------------------------------
 *  Main
 * -------------------------------------------------------------------------------------- */

int main(void)
{
	int rc = 0;
    APEX_DATA3_ACTIVITY_CLASS_t activity_class;
    float cadence_step_per_sec;
    int step_result = 0;

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

    rc = inv_imu_pedometer_enable() ;
   
    if (rc != 0) {
        INV_MSG(INV_MSG_LEVEL_INFO, "pedometer enable Failed. Do nothing %d", rc);
        return rc;
    }

	int i = 0;

    do {
		/* Poll device for data */		
		if (irq_from_device & TO_MASK(INV_GPIO_INT1)) {

			inv_disable_irq();
            
            step_result = inv_imu_pedometer_get_event(&cadence_step_per_sec,&activity_class);

            if(step_result < 0)
                INV_MSG(INV_MSG_LEVEL_ERROR, "pedometer get event failure %d",step_result);
            else
                INV_MSG(INV_MSG_LEVEL_INFO, "step value %d, %.2f steps/sec, type %d",step_result,cadence_step_per_sec,activity_class); 
            
			irq_from_device &= ~TO_MASK(INV_GPIO_INT1);
			inv_enable_irq();

            i++;
		}

        if (i == 100) {
            rc = inv_imu_pedometer_disable();
            i++;
            }
	} while(1);
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
