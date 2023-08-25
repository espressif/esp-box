/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "unity.h"
#include "sdkconfig.h"
#include "driver/i2c.h"
#include "at581x.h"
#include "esp_system.h"
#include "esp_log.h"

static const char *TAG = "AT581X TEST";

#define TEST_MEMORY_LEAK_THRESHOLD (-400)

#define I2C_MASTER_SCL_IO   GPIO_NUM_40     /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO   GPIO_NUM_41     /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM      I2C_NUM_0       /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ  100000          /*!< I2C master clock frequency */
#define RADAR_OUTPUT_IO     GPIO_NUM_21     /*!< radar output IO */

static at581x_dev_handle_t at581x = NULL;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    esp_rom_printf("GPIO[%d] intr, val: %d\n", gpio_num, gpio_get_level(gpio_num));
}

/**
 * @brief i2c master initialization
 */
static void i2c_bus_init(void)
{
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = (gpio_num_t)I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = (gpio_num_t)I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    conf.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;

    esp_err_t ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, ret, "I2C config returned error");

    ret = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, ret, "I2C install returned error");
}

static void i2c_sensor_at581x_init(void)
{
    at581x_dev_handle_t sensor_handle;

    at581x_i2c_config_t i2c_conf = {
        .i2c_port = I2C_MASTER_NUM,
        .i2c_addr = AT581X_ADDRRES_0,
    };

    i2c_bus_init();
    at581x_new_sensor(&i2c_conf, &at581x);
    TEST_ASSERT_NOT_NULL_MESSAGE(at581x, "AT581X create returned NULL");
    at581x_init_sensor(at581x);
}

TEST_CASE("sensor at581x test", "[at581x][iot][sensor]")
{
    esp_err_t ret;

    i2c_sensor_at581x_init();

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.pin_bit_mask = (1ULL << RADAR_OUTPUT_IO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    gpio_set_intr_type(RADAR_OUTPUT_IO, GPIO_INTR_ANYEDGE);
    gpio_install_isr_service(0x00);
    gpio_isr_handler_add(RADAR_OUTPUT_IO, gpio_isr_handler, (void *) RADAR_OUTPUT_IO);

    vTaskDelay(pdMS_TO_TICKS(8000));

    gpio_isr_handler_remove(RADAR_OUTPUT_IO);
    gpio_uninstall_isr_service();

    at581x_del_sensor(at581x);
    ret = i2c_driver_delete(I2C_MASTER_NUM);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

static size_t before_free_8bit;
static size_t before_free_32bit;

static void check_leak(size_t before_free, size_t after_free, const char *type)
{
    ssize_t delta = after_free - before_free;
    printf("MALLOC_CAP_%s: Before %u bytes free, After %u bytes free (delta %d)\n", type, before_free, after_free, delta);
    TEST_ASSERT_MESSAGE(delta >= TEST_MEMORY_LEAK_THRESHOLD, "memory leak");
}

void setUp(void)
{
    before_free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    before_free_32bit = heap_caps_get_free_size(MALLOC_CAP_32BIT);
}

void tearDown(void)
{
    size_t after_free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t after_free_32bit = heap_caps_get_free_size(MALLOC_CAP_32BIT);
    check_leak(before_free_8bit, after_free_8bit, "8BIT");
    check_leak(before_free_32bit, after_free_32bit, "32BIT");
}

void app_main(void)
{
    printf("AT581X TEST \n");
    unity_run_menu();
}
