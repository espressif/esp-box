// Copyright 2020 Espressif Systems (Shanghai) Co. Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "unity.h"
#include "icm42670.h"

static const char *TAG = "IMU TEST";


static void icm42670_test(void *args)
{
    AccDataPacket accData;
    GyroDataPacket gyroData;
    chip_temperature imu_chip_temperature;

    icm42670_init();
    while (1) {
        int ret = icm42670_get_raw_data(&accData, &gyroData, &imu_chip_temperature);
        if (0 == ret) {
            INV_LOG(SENSOR_LOG_LEVEL, "temperature=%.1f", imu_chip_temperature);
            INV_LOG(SENSOR_LOG_LEVEL, "ACC %d   (%.1f, %.1f, %.1f)", accData.accDataSize, accData.databuff[0].x, accData.databuff[0].y, accData.databuff[0].z);
            INV_LOG(SENSOR_LOG_LEVEL, "Gyro%d   (%.1f, %.1f, %.1f)", gyroData.gyroDataSize, gyroData.databuff[0].x, gyroData.databuff[0].y, gyroData.databuff[0].z);
        } else {
            INV_LOG(INV_LOG_LEVEL_ERROR, "read err");
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

TEST_CASE("ICM42670 test", "[imu][box]")
{
    icm42670_test(NULL);
}
