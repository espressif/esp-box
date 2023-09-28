/*
 * SPDX-FileCopyrightText: 2015-2023 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Unlicense OR CC0-1.0
*/

#include <string.h>
#include <inttypes.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>

#include <esp_rmaker_core.h>
#include <esp_rmaker_ota.h>
#include <esp_rmaker_utils.h>
#include <esp_rmaker_schedule.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_common_events.h>

#include "bsp_board.h"

#include "app_pump.h"
#include "app_humidity.h"
#include "app_nvs.h"
#include <app_wifi.h>
extern void ui_watering_update_device_name(const char *);
extern void ui_main_status_bar_set_wifi(bool is_connected);
static const char *TAG = "app_rmaker";

#define ESP_RMAKER_DEVICE_WATERING   "esp.device.watering"

#define ESP_RMAKER_PARAM_WATERING    "esp.param.watering"
#define ESP_RMAKER_PARAM_HUMIDITY    "esp.param.humidity"
#define ESP_RMAKER_PARAM_DURATION    "esp.param.duration"

static esp_err_t watering_write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
                                   const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (ctx) {
        ESP_LOGI(TAG, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }
    const char *device_name = esp_rmaker_device_get_name(device);
    const char *param_name = esp_rmaker_param_get_name(param);


    if (strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0) {
        ESP_LOGI(TAG, "Received value = %s for %s - %s",
                 val.val.b ? "true" : "false", device_name, param_name);
        if (val.val.b) {
            app_pump_watering_start();
        } else {
            app_pump_watering_stop();
        }

    } else if (strcmp(param_name, "Name") == 0) {
        ESP_LOGI(TAG, "Received value = %s for %s - %s",
                 val.val.s, device_name, param_name);
        ui_watering_update_device_name(val.val.s);
    } else if (strcmp(param_name, "MaxDuration") == 0) {
        ESP_LOGI(TAG, "Received value = %d for %s - %s",
                 val.val.i, device_name, param_name);
        app_pump_set_watering_time(val.val.i);
    } else if (strcmp(param_name, "LowerHumidity") == 0) {
        ESP_LOGI(TAG, "Received value = %d for %s - %s",
                 val.val.i, device_name, param_name);
        app_pump_set_lower_humidity(val.val.i);
    } else if (strcmp(param_name, "AutoWatering") == 0) {
        ESP_LOGI(TAG, "Received value = %d for %s - %s",
                 val.val.i, device_name, param_name);
        app_pump_set_auto_watering_enable(val.val.b);
    } else {
        ESP_LOGW(TAG, "Received type = %d for %s - %s",
                 val.type, device_name, param_name);
    }
    esp_rmaker_param_update_and_report(param, val);
    return ESP_OK;
}

esp_rmaker_param_t *esp_rmaker_range_param_create(const char *param_name, int val, int min, int max)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_DURATION,
                                esp_rmaker_int(val), PROP_FLAG_READ | PROP_FLAG_WRITE | PROP_FLAG_PERSIST);
    if (param) {
        esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_SLIDER);
        esp_rmaker_param_add_bounds(param, esp_rmaker_int(min), esp_rmaker_int(max), esp_rmaker_int(1));
    }
    return param;
}

esp_rmaker_param_t *esp_rmaker_current_watering_time_param_create(const char *param_name, int val)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_DURATION,
                                esp_rmaker_int(val), PROP_FLAG_READ);
    return param;
}

static esp_rmaker_param_t *esp_rmaker_humidity_param_create(const char *param_name, float val)
{
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_HUMIDITY,
                                esp_rmaker_float(val), PROP_FLAG_READ);
    return param;
}

static esp_rmaker_device_t *esp_rmaker_watering_device_create(const char *dev_name,
        void *priv_data, bool watering)
{
    esp_rmaker_device_t *device = esp_rmaker_device_create(dev_name, ESP_RMAKER_DEVICE_WATERING, priv_data);
    if (device) {
        //name
        esp_rmaker_device_add_param(device, esp_rmaker_name_param_create(ESP_RMAKER_DEF_NAME_PARAM, dev_name));

        esp_rmaker_device_add_param(device, esp_rmaker_range_param_create("MaxDuration", app_pump_get_watering_time(), 10, 600));
        esp_rmaker_device_add_param(device, esp_rmaker_current_watering_time_param_create("CurrDuration", 0));
        esp_rmaker_device_add_param(device, esp_rmaker_humidity_param_create("Humidity", app_humidity_get_value()));
        esp_rmaker_device_add_param(device, esp_rmaker_power_param_create("AutoWatering", app_pump_get_auto_watering_enable()));
        esp_rmaker_device_add_param(device, esp_rmaker_range_param_create("LowerHumidity", app_pump_get_watering_time(), 40, 84));
        esp_rmaker_param_t *primary = esp_rmaker_power_param_create(ESP_RMAKER_DEF_POWER_NAME, watering);
        esp_rmaker_device_add_param(device, primary);
        esp_rmaker_device_assign_primary_param(device, primary);
    }
    return device;
}

static esp_err_t esp_watering_node_init(void)
{
    /* Initialize the ESP RainMaker Agent.
     * Note that this should be called after app_wifi_init() but before app_wifi_start()
     * */
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = true,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP RainMaker Watering Device", "esp.device.watering");
    ESP_ERROR_CHECK(node == NULL ? ESP_FAIL : ESP_OK);

    esp_rmaker_device_t *watering_device = esp_rmaker_watering_device_create("Watering", NULL, app_pump_is_watering());
    esp_rmaker_device_add_cb(watering_device, watering_write_cb, NULL);
    esp_rmaker_node_add_device(node, watering_device);
    return ESP_OK;
}

static void print_node_config(void)
{
    extern char *esp_rmaker_get_node_config(void);
    char *cfg = esp_rmaker_get_node_config();
    if (cfg) {
        ESP_LOGI(TAG, "%s", cfg);
        free(cfg);
    } else {
        ESP_LOGE(TAG, "get node config failed");
    }
}

/*notify*/
void app_rainmaker_update_watering_state(void *args)
{
    esp_rmaker_param_t *param;
    esp_rmaker_param_val_t val;

    const esp_rmaker_node_t *node = esp_rmaker_get_node();
    esp_rmaker_device_t *device = esp_rmaker_node_get_device_by_name(node, "Watering");
    //power
    param = esp_rmaker_device_get_param_by_name(device, ESP_RMAKER_DEF_POWER_NAME);
    val = esp_rmaker_bool(app_pump_is_watering());
    esp_rmaker_param_update_and_notify(param, val);
    return;
}

void app_rainmaker_update_current_watering_time(void *args)
{
    esp_rmaker_param_t *param;
    esp_rmaker_param_val_t val;

    if (app_pump_curr_watering_time() % 5 != 0) { //report every 5 seconds
        return;
    }
    const esp_rmaker_node_t *node = esp_rmaker_get_node();
    esp_rmaker_device_t *device = esp_rmaker_node_get_device_by_name(node, "Watering");
    //power
    param = esp_rmaker_device_get_param_by_name(device, "CurrDuration");
    val = esp_rmaker_int(app_pump_curr_watering_time());
    esp_rmaker_param_update_and_notify(param, val);
    return;
}

void app_rainmaker_update_watering_humidity(void *args)
{
    esp_rmaker_param_t *param;
    esp_rmaker_param_val_t val;

    const esp_rmaker_node_t *node = esp_rmaker_get_node();
    esp_rmaker_device_t *device = esp_rmaker_node_get_device_by_name(node, "Watering");

    //humidity
    param = esp_rmaker_device_get_param_by_name(device, "Humidity");
    val = esp_rmaker_float(app_humidity_get_value());
    esp_rmaker_param_update_and_notify(param, val);
    return;
}

const char *app_rainmaker_get_device_name(void)
{
    esp_rmaker_param_t *param;
    esp_rmaker_param_val_t *val;

    const esp_rmaker_node_t *node = esp_rmaker_get_node();
    if (node == NULL) {
        return "Device Name";
    }
    esp_rmaker_device_t *device = esp_rmaker_node_get_device_by_name(node, "Watering");

    //humidity
    param = esp_rmaker_device_get_param_by_name(device, "Name");
    val = esp_rmaker_param_get_val(param);
    return val->val.s;
}

/* Event handler for catching RainMaker events */
static void rainmaker_event_handler(void *arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data)
{
    if (event_base == RMAKER_COMMON_EVENT) {
        switch (event_id) {
        case RMAKER_EVENT_REBOOT:
            ESP_LOGI(TAG, "Rebooting in %d seconds.", *((uint8_t *)event_data));
            break;
        case RMAKER_EVENT_WIFI_RESET:
            ESP_LOGI(TAG, "Wi-Fi credentials reset.");
            break;
        case RMAKER_EVENT_FACTORY_RESET:
            ESP_LOGI(TAG, "Node reset to factory defaults.");
            break;
        case RMAKER_MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected.");
            ui_main_status_bar_set_wifi(1);
            break;
        case RMAKER_MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT Disconnected.");
            ui_main_status_bar_set_wifi(0);
            break;
        case RMAKER_MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT Published. Msg id: %d.", *((int *)event_data));
            break;
        default:
            ESP_LOGW(TAG, "Unhandled RainMaker Common Event: %"PRIi32"", event_id);
        }
    } else {
        ESP_LOGW(TAG, "Invalid event received!");
    }
}

static void rmaker_task(void *args)
{
    /* Initialize Wi-Fi. Note that, this should be called before esp_rmaker_node_init()
     */
    app_wifi_init();
    ESP_ERROR_CHECK(esp_event_handler_register(RMAKER_COMMON_EVENT, ESP_EVENT_ANY_ID, &rainmaker_event_handler, NULL));
    esp_watering_node_init();

    /* Enable OTA */
    // esp_rmaker_ota_config_t ota_config = {
    //     .server_cert = ESP_RMAKER_OTA_DEFAULT_SERVER_CERT,
    // };
    // esp_rmaker_ota_enable(&ota_config, OTA_USING_TOPICS);

    /* Enable timezone service. */
    esp_rmaker_timezone_service_enable();

    /* Enable schduel service. */
    esp_rmaker_schedule_enable();

    /* Start rmaker core. */
    esp_rmaker_start();

    /**/
    print_node_config();

    /* Start the Wi-Fi. */
    esp_err_t err = app_wifi_start(POP_TYPE_RANDOM);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not start Wifi");
    }
    app_pump_add_cb_before_watering(app_rainmaker_update_watering_state, NULL);
    app_pump_add_cb_during_watering(app_rainmaker_update_current_watering_time, NULL);
    app_pump_add_cb_after_watering(app_rainmaker_update_watering_state, NULL);
    app_pump_add_cb_after_watering(app_rainmaker_update_watering_humidity, NULL);

    app_humidity_add_watcher(app_rainmaker_update_watering_humidity, NULL);
    ui_watering_update_device_name(app_rainmaker_get_device_name());
    vTaskDelete(NULL);
}

static void wifi_credential_reset(void *handle, void *arg)
{
    ESP_LOGW(TAG, "WiFi credential reset");
    esp_rmaker_wifi_reset(0, 2);
    esp_rmaker_factory_reset(0, 2);
}

esp_err_t app_watering_rmaker_start(void)
{
    bsp_btn_register_callback(BSP_BUTTON_CONFIG, BUTTON_LONG_PRESS_START, wifi_credential_reset, NULL);

    BaseType_t ret_val = xTaskCreatePinnedToCore(rmaker_task, "RMaker Task", 6 * 1024, NULL, 1, NULL, 0);
    ESP_ERROR_CHECK_WITHOUT_ABORT((pdPASS == ret_val) ? ESP_OK : ESP_FAIL);
    return ESP_OK;
}
