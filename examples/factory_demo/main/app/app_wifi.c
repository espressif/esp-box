/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include "esp_bt.h"
#include <esp_netif.h>
#include <qrcode.h>
#include <nvs.h>
#include <nvs_flash.h>

#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>

#include "app_wifi.h"
#include "app_sntp.h"
#include "ui_main.h"
#include "ui_net_config.h"

static const int WIFI_STA_CONNECT_OK    = BIT0;
static const int WIFI_PROV_EVENT_START  = BIT1;
static const int WIFI_PROV_EVENT_STOP   = BIT2;
static const int WIFI_PROV_EVENT_EXIST  = BIT3;
static const int WIFI_PROV_EVENT_STOPED = BIT4;

static bool s_connected = false;
static char s_payload[150] = "";
static const char *TAG = "app_wifi";
static EventGroupHandle_t wifi_event_group;

#define PROV_QR_VERSION         "v1"
#define PROV_TRANSPORT_BLE      "ble"
#define QRCODE_BASE_URL         "https://rainmaker.espressif.com/qrcode.html"

#define CREDENTIALS_NAMESPACE   "rmaker_creds"
#define RANDOM_NVS_KEY          "random"


#if CONFIG_BSP_BOARD_ESP32_S3_BOX_3
/*set the ssid and password via "idf.py menuconfig"*/
#define DEFAULT_LISTEN_INTERVAL CONFIG_EXAMPLE_WIFI_LISTEN_INTERVAL
#define DEFAULT_BEACON_TIMEOUT  CONFIG_EXAMPLE_WIFI_BEACON_TIMEOUT
#endif

#if CONFIG_EXAMPLE_POWER_SAVE_MIN_MODEM
#define DEFAULT_PS_MODE WIFI_PS_MIN_MODEM
#elif CONFIG_EXAMPLE_POWER_SAVE_MAX_MODEM
#define DEFAULT_PS_MODE WIFI_PS_MAX_MODEM
#elif CONFIG_EXAMPLE_POWER_SAVE_NONE
#define DEFAULT_PS_MODE WIFI_PS_NONE
#else
#define DEFAULT_PS_MODE WIFI_PS_NONE
#endif /*CONFIG_POWER_SAVE_MODEM*/

static void app_wifi_print_qr(const char *name)
{
    if (!name) {
        ESP_LOGW(TAG, "Cannot generate QR code payload. Data missing.");
        return;
    }
    snprintf(s_payload, sizeof(s_payload), "{\"ver\":\"%s\",\"name\":\"%s\",\"pop\":\"\",\"transport\":\"%s\"}", PROV_QR_VERSION, name, PROV_TRANSPORT_BLE);
    /* Just highlight */
    // ESP_LOGW(TAG, "Scan this QR code from the ESP BOX app for Provisioning.");
    // qrcode_display(s_payload);
    // ESP_LOGW(TAG, "If QR code is not visible, copy paste the below URL in a browser.\n%s?data=%s", QRCODE_BASE_URL, s_payload);
}

char *app_wifi_get_prov_payload(void)
{
    return s_payload;
}

/* Event handler for catching system events */
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
        case WIFI_PROV_START:
            ESP_LOGI(TAG, "Provisioning started");
            break;
        case WIFI_PROV_CRED_RECV: {
            wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
            ESP_LOGI(TAG, "Received Wi-Fi credentials"
                     "\n\tSSID     : %s\n\tPassword : %s",
                     (const char *) wifi_sta_cfg->ssid,
                     (const char *) wifi_sta_cfg->password);
            break;
        }
        case WIFI_PROV_CRED_FAIL: {
            wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
            ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                     "\n\tPlease reset to factory and retry provisioning",
                     (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                     "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
            esp_wifi_disconnect();
            wifi_prov_mgr_reset_sm_state_on_failure();
            ui_net_config_update_cb(UI_NET_EVT_PROV_CRED_FAIL, NULL);
            break;
        }
        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(TAG, "Provisioning successful");
            break;
        case WIFI_PROV_END:
            ESP_LOGI(TAG, "Provisioning end");
            /* De-initialize manager once provisioning is finished */
            wifi_prov_mgr_deinit();
            break;
        case WIFI_PROV_DEINIT:
            ESP_LOGI(TAG, "Provisioning deinit");
            xEventGroupSetBits(wifi_event_group, WIFI_PROV_EVENT_STOPED);
            ESP_ERROR_CHECK(esp_wifi_set_ps(DEFAULT_PS_MODE));
            break;
        default:
            break;
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ui_net_config_update_cb(UI_NET_EVT_START_CONNECT, NULL);
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_WIFI_READY) {
        ESP_ERROR_CHECK(esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G));
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        s_connected = 1;
        ui_acquire();
        ui_main_status_bar_set_wifi(s_connected);
        ui_release();
        /* Signal main application to continue execution */
        xEventGroupSetBits(wifi_event_group, WIFI_STA_CONNECT_OK);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
        esp_wifi_connect();
        s_connected = 0;
        ui_acquire();
        ui_main_status_bar_set_wifi(s_connected);
        ui_release();
        ui_net_config_update_cb(UI_NET_EVT_START_CONNECT, NULL);
    }
}

static void wifi_init_sta()
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_ps(DEFAULT_PS_MODE));
    ESP_ERROR_CHECK(esp_wifi_start());
}

/* Free random_bytes after use only if function returns ESP_OK */
static esp_err_t read_random_bytes_from_nvs(uint8_t **random_bytes, size_t *len)
{
    nvs_handle handle;
    esp_err_t err;
    *len = 0;

    if ((err = nvs_open_from_partition(CONFIG_ESP_RMAKER_FACTORY_PARTITION_NAME, CREDENTIALS_NAMESPACE,
                                       NVS_READONLY, &handle)) != ESP_OK) {
        ESP_LOGD(TAG, "NVS open for %s %s %s failed with error %d", CONFIG_ESP_RMAKER_FACTORY_PARTITION_NAME, CREDENTIALS_NAMESPACE, RANDOM_NVS_KEY, err);
        return ESP_FAIL;
    }

    if ((err = nvs_get_blob(handle, RANDOM_NVS_KEY, NULL, len)) != ESP_OK) {
        ESP_LOGD(TAG, "Error %d. Failed to read key %s.", err, RANDOM_NVS_KEY);
        nvs_close(handle);
        return ESP_ERR_NOT_FOUND;
    }

    *random_bytes = calloc(*len, 1);
    if (*random_bytes) {
        nvs_get_blob(handle, RANDOM_NVS_KEY, *random_bytes, len);
        nvs_close(handle);
        return ESP_OK;
    }
    nvs_close(handle);
    return ESP_ERR_NO_MEM;
}

static esp_err_t get_device_service_name(char *service_name, size_t max)
{
    uint8_t *nvs_random = NULL;
    const char *ssid_prefix = "BOX_";
    size_t nvs_random_size = 0;
    if ((read_random_bytes_from_nvs(&nvs_random, &nvs_random_size) != ESP_OK) || nvs_random_size < 3) {
        uint8_t eth_mac[6];
        esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
        snprintf(service_name, max, "%s%02x%02x%02x", ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
    } else {
        snprintf(service_name, max, "%s%02x%02x%02x", ssid_prefix, nvs_random[nvs_random_size - 3],
                 nvs_random[nvs_random_size - 2], nvs_random[nvs_random_size - 1]);
    }

    if (nvs_random) {
        free(nvs_random);
    }
    return ESP_OK;
}

void app_wifi_init(void)
{
    esp_netif_init();

    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_event_group = xEventGroupCreate();

    /* Register our event handler for Wi-Fi, IP and Provisioning related events */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    /* Initialize Wi-Fi including netif with default config */
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

#if CONFIG_BSP_BOARD_ESP32_S3_BOX_3
    wifi_config_t wifi_cfg;
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_cfg));
    wifi_cfg.sta.listen_interval = DEFAULT_LISTEN_INTERVAL;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_inactive_time(WIFI_IF_STA, DEFAULT_BEACON_TIMEOUT));
#endif
}

esp_err_t app_wifi_prov_start(void)
{
    bool provisioned = false;
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
    if (!provisioned) {
        ESP_LOGI(TAG, "app_wifi_prov_start");
        return xEventGroupSetBits(wifi_event_group, WIFI_PROV_EVENT_START);
    } else {
        return ESP_FAIL;
    }
}

esp_err_t app_wifi_prov_stop(void)
{
    bool provisioned = false;
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
    if (!provisioned) {
        ESP_LOGI(TAG, "app_wifi_prov_stop");
        xEventGroupSetBits(wifi_event_group, WIFI_PROV_EVENT_STOP);
        xEventGroupClearBits(wifi_event_group, WIFI_PROV_EVENT_STOPED);
        xEventGroupWaitBits(wifi_event_group, WIFI_PROV_EVENT_STOPED, pdFALSE, pdFALSE, pdMS_TO_TICKS(1000));
        return ESP_OK;
    } else {
        return ESP_FAIL;
    }
}

esp_err_t app_wifi_start(void)
{
    esp_err_t err;

    ui_net_config_update_cb(UI_NET_EVT_START, NULL);

    xEventGroupClearBits(wifi_event_group, WIFI_STA_CONNECT_OK);
    xEventGroupClearBits(wifi_event_group, WIFI_PROV_EVENT_START);
    xEventGroupClearBits(wifi_event_group, WIFI_PROV_EVENT_STOP);
    xEventGroupClearBits(wifi_event_group, WIFI_PROV_EVENT_EXIST);

    /* If device is not yet provisioned start provisioning service */
    bool provisioned = false;
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
    if (!provisioned) {
        while (1) {
            ESP_LOGI(TAG, "waiting provisioning");
            xEventGroupWaitBits(wifi_event_group, WIFI_PROV_EVENT_START, pdFALSE, pdFALSE, portMAX_DELAY);
            xEventGroupClearBits(wifi_event_group, WIFI_PROV_EVENT_START);

            /* Provisioning framework initialization */
            wifi_prov_mgr_config_t config = {
                .scheme = wifi_prov_scheme_ble,
                .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BT,
            };
            ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

            ui_net_config_update_cb(UI_NET_EVT_START_PROV, NULL);
            err = esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
            if (err != ESP_OK) {
                ui_net_config_update_cb(UI_NET_EVT_PROV_SET_PS_FAIL, NULL);
                continue;
            };

            /* Get bluetooth broadcast name */
            char service_name[12];
            err = get_device_service_name(service_name, sizeof(service_name));
            if (err != ESP_OK) {
                ui_net_config_update_cb(UI_NET_EVT_PROV_GET_NAME_FAIL, NULL);
                continue;
            };

            uint8_t mfg[] = { 0xe5, 0x02, 'N', 'o', 'v', 'a', 0x00, 0x02, 0x00, 0xF0, 0x01, 0x00 };
            err = wifi_prov_scheme_ble_set_mfg_data(mfg, sizeof(mfg));
            if (err != ESP_OK) {
                ui_net_config_update_cb(UI_NET_EVT_PROV_SET_MFG_FAIL, NULL);
                continue;
            }

            /* Start provisioning */
            err = wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1, NULL, service_name, NULL);
            if (err != ESP_OK) {
                ui_net_config_update_cb(UI_NET_EVT_PROV_START_FAIL, NULL);
                continue;
            }

            app_wifi_print_qr(service_name);
            ui_net_config_update_cb(UI_NET_EVT_GET_NAME, NULL);
            ESP_LOGI(TAG, "Provisioning Started. Name : %s", service_name);

            xEventGroupWaitBits(wifi_event_group, WIFI_STA_CONNECT_OK | WIFI_PROV_EVENT_STOP, pdFALSE, pdFALSE, portMAX_DELAY);

            if (WIFI_STA_CONNECT_OK & xEventGroupGetBits(wifi_event_group) ) {
                ESP_LOGI(TAG, "Wi-Fi Provisioned OK, stoped:%d", WIFI_PROV_EVENT_STOPED & xEventGroupGetBits(wifi_event_group));
                xEventGroupWaitBits(wifi_event_group, WIFI_PROV_EVENT_STOPED, pdFALSE, pdFALSE, pdMS_TO_TICKS(10000));
                esp_bt_mem_release(ESP_BT_MODE_BTDM);
                ESP_LOGI(TAG, "BLE memory released");
                break;
            } else if (WIFI_PROV_EVENT_STOP & xEventGroupGetBits(wifi_event_group) ) {
                ESP_LOGI(TAG, "Wi-Fi Provisioned Stop");
                wifi_prov_mgr_stop_provisioning();
                xEventGroupClearBits(wifi_event_group, WIFI_PROV_EVENT_STOP);
                continue;
            }
        }
    } else {
        ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi STA");
        wifi_init_sta();
        xEventGroupSetBits(wifi_event_group, WIFI_PROV_EVENT_EXIST);
    }

    xEventGroupWaitBits(wifi_event_group, WIFI_STA_CONNECT_OK, pdFALSE, pdFALSE, pdMS_TO_TICKS(1000 * 80));
    if ( 0 == (WIFI_STA_CONNECT_OK & xEventGroupGetBits(wifi_event_group)) ) {
        if (WIFI_PROV_EVENT_EXIST & xEventGroupGetBits(wifi_event_group) ) {
            ESP_LOGI(TAG, "Wi-Fi Connect Failed");
            esp_wifi_disconnect();
            ui_net_config_update_cb(UI_NET_EVT_CONNECT_FAILED, NULL);
        }
        return ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "Wi-Fi Connect");
        ui_net_config_update_cb(UI_NET_EVT_WIFI_CONNECTED, NULL);
        app_sntp_init();

        printf("Current Free Memory\t%d\t\t%d\n",
               heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
               heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        printf("Largest Free Block\t%d\t\t%d\n",
               heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
               heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
        return ESP_OK;
    }
}

bool app_wifi_is_connected(void)
{
    return s_connected;
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
