/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_check.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "app_wifi.h"
#include "settings.h"

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/


#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT      BIT0
#define WIFI_FAIL_BIT           BIT1

#define portTICK_RATE_MS        10

static const char *TAG = "wifi station";
static int s_retry_num = 0;
static bool s_reconnect = true;

static bool wifi_connected = false;
static QueueHandle_t wifi_event_queue = NULL;

scan_info_t scan_info_result = {
    .scan_done = WIFI_SCAN_IDLE,
    .ap_count = 0,
};

WiFi_Connect_Status wifi_connected_already(void)
{
    WiFi_Connect_Status status;
    if (true == wifi_connected) {
        status = WIFI_STATUS_CONNECTED_OK;
    } else {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            status = WIFI_STATUS_CONNECTING;
        } else {
            status = WIFI_STATUS_CONNECTED_FAILED;
        }
    }
    return status;
}

esp_err_t app_wifi_get_wifi_ssid(char *ssid, size_t len)
{
    wifi_config_t wifi_cfg;
    if (esp_wifi_get_config(WIFI_IF_STA, &wifi_cfg) != ESP_OK) {
        return ESP_FAIL;
    }
    strncpy(ssid, (const char *)wifi_cfg.sta.ssid, len);
    return ESP_OK;
}

esp_err_t send_network_event(net_event_t event)
{
    net_event_t eventOut = event;
    BaseType_t ret_val = xQueueSend(wifi_event_queue, &eventOut, 0);

    if (NET_EVENT_RECONNECT == event) {
        wifi_connected = false;
    }

    ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_ERR_INVALID_STATE,
                        TAG, "The last event has not been processed yet");
    return ESP_OK;
}

/* Initialize Wi-Fi as sta and set scan method */
static void wifi_scan(void)
{
    uint16_t number = DEFAULT_SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    app_wifi_state_set(WIFI_SCAN_BUSY);

    esp_err_t ret = esp_wifi_scan_start(NULL, true);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_LOGI(TAG, "Total APs scanned = %u, ret:%d", ap_count, ret);

    for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++) {
        ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
        /*
        ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
        print_auth_mode(ap_info[i].authmode);
        if (ap_info[i].authmode != WIFI_AUTH_WEP) {
            print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
        }
        ESP_LOGI(TAG, "Channel \t\t%d\n", ap_info[i].primary);
        */
    }

    if (ap_count && (ESP_OK == ret)) {
        scan_info_result.ap_count = (ap_count < DEFAULT_SCAN_LIST_SIZE) ? ap_count : DEFAULT_SCAN_LIST_SIZE;
        memcpy(&scan_info_result.ap_info[0], &ap_info[0], sizeof(wifi_ap_record_t)*scan_info_result.ap_count);
    } else {
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGI(TAG, "failed return");
    }
    app_wifi_state_set(WIFI_SCAN_RENEW);
}

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        send_network_event(NET_EVENT_POWERON_SCAN);
        ESP_LOGI(TAG, "start connect to the AP");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_reconnect && ++s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            ESP_LOGI(TAG, "sta disconnect, retry attempt %d...", s_retry_num);
        } else {
            ESP_LOGI(TAG, "sta disconnected");
        }
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        wifi_connected = false;
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        wifi_connected = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_reconnect_sta(void)
{
    int bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, 0, 1, 0);

    wifi_config_t wifi_config = { 0 };

    sys_param_t *sys_param = settings_get_parameter();
    memcpy(wifi_config.sta.ssid, sys_param->ssid, sizeof(wifi_config.sta.ssid));
    memcpy(wifi_config.sta.password, sys_param->password, sizeof(wifi_config.sta.password));
    //ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );

    if (bits & WIFI_CONNECTED_BIT) {
        s_reconnect = false;
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_ERROR_CHECK( esp_wifi_disconnect() );
        xEventGroupWaitBits(s_wifi_event_group, WIFI_FAIL_BIT, 0, 1, portTICK_RATE_MS);
    }

    s_reconnect = true;
    s_retry_num = 0;
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    esp_wifi_connect();

    ESP_LOGI(TAG, "wifi_reconnect_sta finished.%s, %s", \
            wifi_config.sta.ssid, wifi_config.sta.password);

    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, 0, 1, 5000 / portTICK_RATE_MS);
}

static void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    ESP_EVENT_ANY_ID,
                    &event_handler,
                    NULL,
                    &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                    IP_EVENT_STA_GOT_IP,
                    &event_handler,
                    NULL,
                    &instance_got_ip));

    wifi_config_t wifi_config = { 
        .sta = {
            .ssid = {0}, 
            .password = {0},
        },
    };
    sys_param_t *sys_param = settings_get_parameter();
    memcpy(wifi_config.sta.ssid, sys_param->ssid, sizeof(wifi_config.sta.ssid));
    memcpy(wifi_config.sta.password, sys_param->password, sizeof(wifi_config.sta.password));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );
    ESP_LOGI(TAG, "wifi_init_sta finished.%s, %s", \
             wifi_config.sta.ssid, wifi_config.sta.password);
}

static void network_task(void *args)
{
    net_event_t net_event;

    wifi_init_sta();

    while (1) {
        if (pdPASS == xQueueReceive(wifi_event_queue, &net_event, portTICK_RATE_MS / 5)) {
            switch (net_event) {
            case NET_EVENT_RECONNECT:
                ESP_LOGI(TAG, "NET_EVENT_RECONNECT");
                wifi_reconnect_sta();
                break;
            case NET_EVENT_SCAN:
                ESP_LOGI(TAG, "NET_EVENT_SCAN");
                wifi_scan();
                break;
            case NET_EVENT_NTP:
                ESP_LOGI(TAG, "NET_EVENT_NTP");
                break;
            case NET_EVENT_WEATHER:
                ESP_LOGI(TAG, "NET_EVENT_WEATHER");
                break;

            case NET_EVENT_POWERON_SCAN:
                ESP_LOGI(TAG, "NET_EVENT_POWERON_SCAN");
                wifi_scan();
                esp_wifi_connect();
                wifi_connected = false;
                break;
            default:
                break;
            }
        }
    }
    vTaskDelete(NULL);
}

bool app_wifi_lock(uint32_t timeout_ms)
{
    assert(scan_info_result.wifi_mux && "bsp_display_start must be called first");

    const TickType_t timeout_ticks = (timeout_ms == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(scan_info_result.wifi_mux, timeout_ticks) == pdTRUE;
}

void app_wifi_unlock(void)
{
    assert(scan_info_result.wifi_mux && "bsp_display_start must be called first");
    xSemaphoreGiveRecursive(scan_info_result.wifi_mux);
}

void app_wifi_state_set(wifi_scan_status_t status)
{
    app_wifi_lock(0);
    scan_info_result.scan_done = status;
    app_wifi_unlock();
}

void app_network_start(void)
{
    BaseType_t ret_val;

    scan_info_result.wifi_mux = xSemaphoreCreateRecursiveMutex();
    ESP_ERROR_CHECK_WITHOUT_ABORT((scan_info_result.wifi_mux) ? ESP_OK : ESP_FAIL);

    wifi_event_queue = xQueueCreate(4, sizeof(net_event_t));
    ESP_ERROR_CHECK_WITHOUT_ABORT((wifi_event_queue) ? ESP_OK : ESP_FAIL);

    ret_val = xTaskCreatePinnedToCore(network_task, "NetWork Task", 5 * 1024, NULL, 1, NULL, 0);
    ESP_ERROR_CHECK_WITHOUT_ABORT((pdPASS == ret_val) ? ESP_OK : ESP_FAIL);
}
