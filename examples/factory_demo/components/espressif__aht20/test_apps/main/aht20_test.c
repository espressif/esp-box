/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "unity.h"
#include "driver/i2c.h"
#include "aht20.h"
#include "esp_system.h"
#include "esp_log.h"

static const char *TAG = "aht20 test";

#define TEST_MEMORY_LEAK_THRESHOLD (-400)

#define I2C_MASTER_SCL_IO   GPIO_NUM_40     /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO   GPIO_NUM_41     /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM      I2C_NUM_0       /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ  100000          /*!< I2C master clock frequency */

static aht20_dev_handle_t aht20 = NULL;

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

static void i2c_sensor_ath20_init(void)
{
    aht20_i2c_config_t i2c_conf = {
        .i2c_port = I2C_MASTER_NUM,
        .i2c_addr = AHT20_ADDRRES_0,
    };

    i2c_bus_init();
    aht20_new_sensor(&i2c_conf, &aht20);
    TEST_ASSERT_NOT_NULL_MESSAGE(aht20, "AHT20 create returned NULL");
    aht20_init_sensor(aht20);
}

TEST_CASE("sensor aht20 test", "[aht20][iot][sensor]")
{
    esp_err_t ret = ESP_OK;
    uint32_t temperature_raw, humidity_raw;
    float temperature_s = 0;
    uint8_t humidity_s = 0;

    i2c_sensor_ath20_init();

    aht20_read_temperature_humidity(aht20, &temperature_raw, &temperature_s, &humidity_raw, &humidity_s);
    ESP_LOGI(TAG, "%-20s: %d %%", "humidity value is", humidity_s);
    ESP_LOGI(TAG, "%-20s: %2.2f degC", "temperature value is", temperature_s);

    aht20_del_sensor(aht20);
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
    printf("AHT20 TEST \n");
    unity_run_menu();
}
