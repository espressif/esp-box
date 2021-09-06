/**
 * @file app_network.c
 * @brief 
 * @version 0.1
 * @date 2021-09-26
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

#include <stdio.h>
#include "app_server.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/apps/netbiosns.h"
#include "mdns.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

static const char *TAG = "app_network";

static void initialise_mdns(const char *file_name)
{
    mdns_init();
    mdns_hostname_set("esp-cube");
    mdns_instance_name_set("esp-cube");

    mdns_txt_item_t serviceTxtData[] = {
        {"board", "ESP32-S3-Cube"},
        {"path", "/"}
    };

    ESP_ERROR_CHECK(
        mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, serviceTxtData,
        sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}

static void network_task(void *pvParam)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    initialise_mdns("esp-cube");
    netbiosns_init();
    netbiosns_set_name("esp-cube");

    ESP_ERROR_CHECK(example_connect());
    start_rest_server("/spiffs/web");

    vTaskDelete(NULL);
}

esp_err_t app_network_start(const char *host_name)
{
    /* Create audio detect task. Detect data fetch from buffer */
    BaseType_t ret_val = xTaskCreatePinnedToCore(
        (TaskFunction_t)        network_task,
        (const char * const)    "Network Task",
        (const uint32_t)        4 * 1024,
        (void * const)          NULL,
        (UBaseType_t)           1,
        (TaskHandle_t * const)  NULL,
        (const BaseType_t)      0);
    if (pdPASS != ret_val) {
        ESP_LOGE(TAG, "Failed create Network task");
        return ESP_FAIL;
    }

    return ESP_OK;
}
