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
#include <string.h>
#include "app_server.h"
#include "app_network.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/apps/netbiosns.h"
#include "mdns.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "ui_main.h"

static const char *TAG = "app_network";

static void initialise_mdns(const char *file_name)
{
    mdns_init();
    mdns_hostname_set("esp-box");
    mdns_instance_name_set("esp-box");

    mdns_txt_item_t serviceTxtData[] = {
        {"board", "ESP32-S3-Box"},
        {"path", "/"}
    };

    ESP_ERROR_CHECK(
        mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, serviceTxtData,
        sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}

static void soft_ap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (WIFI_EVENT_AP_STACONNECTED == event_id) {
        ui_network_set_state(true);
    }

    if (WIFI_EVENT_AP_STADISCONNECTED == event_id) {
        ui_network_set_state(false);
    }
}

static void start_soft_ap(void)
{
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &soft_ap_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pairwise_cipher = WIFI_CIPHER_TYPE_CCMP,
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

static void network_task(void *pvParam)
{
    char *host_name = (char *) pvParam;

    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    initialise_mdns(host_name);
    netbiosns_init();
    netbiosns_set_name(host_name);

    /* Start soft-AP */
    start_soft_ap();

    /* Start example connection if you want to make ESP32-S3 as station */
    // ESP_ERROR_CHECK(example_connect());

    start_rest_server("/spiffs/web");

    vTaskDelete(NULL);
}

esp_err_t app_network_start(const char *host_name)
{
    BaseType_t ret_val = xTaskCreatePinnedToCore(
        (TaskFunction_t)        network_task,
        (const char * const)    "Network Task",
        (const uint32_t)        4 * 1024,
        (void * const)          host_name,
        (UBaseType_t)           1,
        (TaskHandle_t * const)  NULL,
        (const BaseType_t)      0);
    if (pdPASS != ret_val) {
        ESP_LOGE(TAG, "Failed create Network task");
        return ESP_FAIL;
    }

    return ESP_OK;
}
