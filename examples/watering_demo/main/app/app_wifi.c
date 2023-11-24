/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
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
#include <esp_idf_version.h>
#include "esp_bt.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 1, 0)
// Features supported in 4.1+
#define ESP_NETIF_SUPPORTED
#endif

#ifdef ESP_NETIF_SUPPORTED
#include <esp_netif.h>
#else
#include <tcpip_adapter.h>
#endif

#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>
#ifdef CONFIG_APP_WIFI_PROV_TRANSPORT_BLE
1
#include <wifi_provisioning/scheme_ble.h>
#else /* CONFIG_APP_WIFI_PROV_TRANSPORT_SOFTAP */
#include <wifi_provisioning/scheme_softap.h>
#endif /* CONFIG_APP_WIFI_PROV_TRANSPORT_BLE */

#include <qrcode.h>
#include <nvs.h>
#include <nvs_flash.h>
#include "app_wifi.h"

static bool s_connected = false;
static char s_payload[150] = "";
static const char *TAG = "app_wifi";
static const int WIFI_CONNECTED_EVENT = BIT0;
static EventGroupHandle_t wifi_event_group;

// #define CONFIG_APP_WIFI_SHOW_DEMO_INTRO_TEXT
// #define CONFIG_APP_WIFI_PROV_SHOW_QR

#define PROV_QR_VERSION "v1"

#define PROV_TRANSPORT_SOFTAP   "softap"
#define PROV_TRANSPORT_BLE      "ble"
#define QRCODE_BASE_URL     "https://rainmaker.espressif.com/qrcode.html"

#define CREDENTIALS_NAMESPACE   "rmaker_creds"
#define RANDOM_NVS_KEY          "random"

#ifdef CONFIG_APP_WIFI_SHOW_DEMO_INTRO_TEXT

#define ESP_RAINMAKER_GITHUB_EXAMPLES_PATH  "https://github.com/espressif/esp-rainmaker/blob/master/examples"
#define ESP_RAINMAKER_INTRO_LINK    "https://rainmaker.espressif.com"
#define ESP_RMAKER_PHONE_APP_LINK   "http://bit.ly/esp-rmaker"
char esp_rainmaker_ascii_art[] = \
                                 "  ______  _____ _____    _____            _____ _   _ __  __          _  ________ _____\n"\
                                 " |  ____|/ ____|  __ \\  |  __ \\     /\\   |_   _| \\ | |  \\/  |   /\\   | |/ /  ____|  __ \\\n"\
                                 " | |__  | (___ | |__) | | |__) |   /  \\    | | |  \\| | \\  / |  /  \\  | ' /| |__  | |__) |\n"\
                                 " |  __|  \\___ \\|  ___/  |  _  /   / /\\ \\   | | | . ` | |\\/| | / /\\ \\ |  < |  __| |  _  /\n"\
                                 " | |____ ____) | |      | | \\ \\  / ____ \\ _| |_| |\\  | |  | |/ ____ \\| . \\| |____| | \\ \\\n"\
                                 " |______|_____/|_|      |_|  \\_\\/_/    \\_\\_____|_| \\_|_|  |_/_/    \\_\\_|\\_\\______|_|  \\_\\\n";

static void intro_print(bool provisioned)
{
    printf("####################################################################################################\n");
    printf("%s\n", esp_rainmaker_ascii_art);
    printf("Welcome to ESP RainMaker %s demo application!\n", RMAKER_DEMO_PROJECT_NAME);
    if (!provisioned) {
        printf("Follow these steps to get started:\n");
        printf("1. Download the ESP RainMaker phone app by visiting this link from your phone's browser:\n\n");
        printf("   %s\n\n", ESP_RMAKER_PHONE_APP_LINK);
        printf("2. Sign up and follow the steps on screen to add the device to your Wi-Fi network.\n");
        printf("3. You are now ready to use the device and control it locally as well as remotely.\n");
        printf("   You can also use the Boot button on the board to control your device.\n");
    }
    printf("\nIf you want to reset Wi-Fi credentials, or reset to factory, press and hold the Boot button.\n");
    printf("\nThis application uses ESP RainMaker, which is based on ESP IDF.\n");
    printf("Check out the source code for this application here:\n   %s/%s\n",
           ESP_RAINMAKER_GITHUB_EXAMPLES_PATH, RMAKER_DEMO_PROJECT_NAME);
    printf("\nPlease visit %s for additional information.\n\n", ESP_RAINMAKER_INTRO_LINK);
    printf("####################################################################################################\n");
}

#else

static void intro_print(bool provisioned)
{
    /* Do nothing */
}

#endif /* !APP_WIFI_SHOW_DEMO_INTRO_TEXT */

static void app_wifi_print_qr(const char *name, const char *pop, const char *transport)
{
    if (!name || !pop || !transport) {
        ESP_LOGW(TAG, "Cannot generate QR code payload. Data missing.");
        return;
    }

    snprintf(s_payload, sizeof(s_payload), "{\"ver\":\"%s\",\"name\":\"%s\"" \
             ",\"pop\":\"%s\",\"transport\":\"%s\"}",
             PROV_QR_VERSION, name, pop, transport);
#ifdef CONFIG_APP_WIFI_PROV_SHOW_QR
    ESP_LOGI(TAG, "Scan this QR code from the ESP RainMaker phone app for Provisioning.");
    qrcode_display(s_payload);
#endif /* CONFIG_APP_WIFI_PROV_SHOW_QR */
    ESP_LOGI(TAG, "If QR code is not visible, copy paste the below URL in a browser.\n%s?data=%s", QRCODE_BASE_URL, s_payload);
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
            wifi_prov_mgr_reset_sm_state_on_failure();
            break;
        }
        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(TAG, "Provisioning successful");
            break;
        case WIFI_PROV_END:
            ESP_LOGI(TAG, "Provisioning end");
            esp_bt_controller_deinit();
            esp_bt_controller_disable();

            /* De-initialize manager once provisioning is finished */
            wifi_prov_mgr_deinit();
            break;
        default:
            break;
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        /* Signal main application to continue execution */
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_EVENT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
        esp_wifi_connect();
    }
}

static void wifi_init_sta()
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
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
    const char *ssid_prefix = "BOX";
    size_t nvs_random_size = 0;
    if ((read_random_bytes_from_nvs(&nvs_random, &nvs_random_size) != ESP_OK) || nvs_random_size < 3) {
        uint8_t eth_mac[6];
        esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
        snprintf(service_name, max, "%s_%02x%02x%02x", ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
    } else {
        snprintf(service_name, max, "%s_%02x%02x%02x", ssid_prefix, nvs_random[nvs_random_size - 3],
                 nvs_random[nvs_random_size - 2], nvs_random[nvs_random_size - 1]);
    }

    if (nvs_random) {
        free(nvs_random);
    }
    return ESP_OK;
}


static esp_err_t get_device_pop(char *pop, size_t max, app_wifi_pop_type_t pop_type)
{
    if (!pop || !max) {
        return ESP_ERR_INVALID_ARG;
    }

    if (pop_type == POP_TYPE_MAC) {
        uint8_t eth_mac[6];
        esp_err_t err = esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
        if (err == ESP_OK) {
            snprintf(pop, max, "%02x%02x%02x%02x", eth_mac[2], eth_mac[3], eth_mac[4], eth_mac[5]);
            return ESP_OK;
        } else {
            return err;
        }
    } else if (pop_type == POP_TYPE_RANDOM) {
        uint8_t *nvs_random;
        size_t nvs_random_size = 0;
        if ((read_random_bytes_from_nvs(&nvs_random, &nvs_random_size) != ESP_OK) || nvs_random_size < 4) {
            return ESP_ERR_NOT_FOUND;
        } else {
            snprintf(pop, max, "%02x%02x%02x%02x", nvs_random[0], nvs_random[1], nvs_random[2], nvs_random[3]);
            free(nvs_random);
            return ESP_OK;
        }
    } else {
        return ESP_ERR_INVALID_ARG;
    }
}

void app_wifi_init(void)
{
    /* Initialize TCP/IP */
#ifdef ESP_NETIF_SUPPORTED
    esp_netif_init();
#else
    tcpip_adapter_init();
#endif

    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_event_group = xEventGroupCreate();

    /* Register our event handler for Wi-Fi, IP and Provisioning related events */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    /* Initialize Wi-Fi including netif with default config */
#ifdef ESP_NETIF_SUPPORTED
    esp_netif_create_default_wifi_sta();
#endif
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}

#define swap_16(x) ((uint16_t)((((x)&0xff00) >> 8) | (((x)&0x00ff) << 8)))
static esp_err_t init_mfg_data(void)
{
    uint8_t mfg[12] = { 0xe5, 0x02 };
    const char *app_id = "Nova";
    sscanf(app_id, "%s", &mfg[2]);

    uint16_t manufacturer_id = swap_16(2);
    memcpy(&mfg[6], &manufacturer_id, 2);

    uint16_t device_code = swap_16(241);
    memcpy(&mfg[8], &device_code, 2);

    uint8_t device_subtype_code = 1;
    memcpy(&mfg[10], &device_subtype_code, 1);

    uint8_t device_extra_code = 1;
    memcpy(&mfg[11], &device_extra_code, 1);

    ESP_LOG_BUFFER_HEXDUMP(TAG, mfg, 12, ESP_LOG_INFO);
    return wifi_prov_scheme_ble_set_mfg_data(mfg, sizeof(mfg));
}

esp_err_t app_wifi_start(app_wifi_pop_type_t pop_type)
{
    /* Configuration for the provisioning manager */
    wifi_prov_mgr_config_t config = {
        /* What is the Provisioning Scheme that we want ?
         * wifi_prov_scheme_softap or wifi_prov_scheme_ble */
#ifdef CONFIG_APP_WIFI_PROV_TRANSPORT_BLE
        .scheme = wifi_prov_scheme_ble,
#else /* CONFIG_APP_WIFI_PROV_TRANSPORT_SOFTAP */
        .scheme = wifi_prov_scheme_softap,
#endif /* CONFIG_APP_WIFI_PROV_TRANSPORT_BLE */

        /* Any default scheme specific event handler that you would
         * like to choose. Since our example application requires
         * neither BT nor BLE, we can choose to release the associated
         * memory once provisioning is complete, or not needed
         * (in case when device is already provisioned). Choosing
         * appropriate scheme specific event handler allows the manager
         * to take care of this automatically. This can be set to
         * WIFI_PROV_EVENT_HANDLER_NONE when using wifi_prov_scheme_softap*/
#ifdef CONFIG_APP_WIFI_PROV_TRANSPORT_BLE
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
#else /* CONFIG_APP_WIFI_PROV_TRANSPORT_SOFTAP */
        .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE,
#endif /* CONFIG_APP_WIFI_PROV_TRANSPORT_BLE */
    };

    /* Initialize provisioning manager with the
     * configuration parameters set above */
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

    bool provisioned = false;
    /* Let's find out if the device is provisioned */
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
    /* If device is not yet provisioned start provisioning service */
    if (!provisioned) {
        ESP_LOGI(TAG, "Starting provisioning");
#ifdef ESP_NETIF_SUPPORTED
        esp_netif_create_default_wifi_ap();
#endif

        esp_err_t err;
        /*
        * ble mfg data
        */
        err = init_mfg_data();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error: %d. Failed to init ble mfg data.", err);
            return err;
        }
        /* What is the Device Service Name that we want
         * This translates to :
         *     - Wi-Fi SSID when scheme is wifi_prov_scheme_softap
         *     - device name when scheme is wifi_prov_scheme_ble
         */
        char service_name[12];
        get_device_service_name(service_name, sizeof(service_name));

        /* What is the security level that we want (0 or 1):
         *      - WIFI_PROV_SECURITY_0 is simply plain text communication.
         *      - WIFI_PROV_SECURITY_1 is secure communication which consists of secure handshake
         *          using X25519 key exchange and proof of possession (pop) and AES-CTR
         *          for encryption/decryption of messages.
         */
        wifi_prov_security_t security = WIFI_PROV_SECURITY_1;

        /* Do we want a proof-of-possession (ignored if Security 0 is selected):
         *      - this should be a string with length > 0
         *      - NULL if not used
         */
        char pop[9] = {0};
        err = get_device_pop(pop, sizeof(pop), pop_type);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error: %d. Failed to get PoP from NVS, Please perform Claiming.", err);
            return err;
        }

        /* What is the service key (Wi-Fi password)
         * NULL = Open network
         * This is ignored when scheme is wifi_prov_scheme_ble
         */
        /* Start provisioning service */
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, NULL, service_name, NULL));
        /* Print QR code for provisioning */
#ifdef CONFIG_APP_WIFI_PROV_TRANSPORT_BLE
        app_wifi_print_qr(service_name, pop, PROV_TRANSPORT_BLE);
#else /* CONFIG_APP_WIFI_PROV_TRANSPORT_SOFTAP */
        app_wifi_print_qr(service_name, pop, PROV_TRANSPORT_SOFTAP);
#endif /* CONFIG_APP_WIFI_PROV_TRANSPORT_BLE */
        intro_print(provisioned);
        ESP_LOGI(TAG, "Provisioning Started. Name : %s, POP : %s", service_name, pop);
    } else {
        ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi STA");
        intro_print(provisioned);
        /* We don't need the manager as device is already provisioned,
         * so let's release it's resources */
        wifi_prov_mgr_deinit();

        /* Start Wi-Fi station */
        wifi_init_sta();
    }
    /* Wait for Wi-Fi connection */
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_EVENT, false, true, portMAX_DELAY);

    s_connected = true;

    return ESP_OK;
}

bool app_wifi_is_connected(void)
{
    return s_connected;
}
