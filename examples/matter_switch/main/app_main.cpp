/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_matter.h>
#include <esp_matter_console.h>
#include <esp_matter_ota.h>

#include <app_priv.h>
#include <app_reset.h>
#include <box_main.h>
#include <app/server/Server.h>

static const char *TAG = "app_main";
uint16_t switch_endpoint_id = 0;

using namespace esp_matter;
using namespace esp_matter::attribute;
using namespace esp_matter::endpoint;
static bool isCommissioningComplete = false;

static SemaphoreHandle_t box_sem = NULL;

void start_box(void)
{
    xSemaphoreGive(box_sem);
}

static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg)
{
    switch (event->Type) {
    case chip::DeviceLayer::DeviceEventType::kInterfaceIpAddressChanged:
        ESP_LOGI(TAG, "Interface IP Address Changed");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
        ESP_LOGI(TAG, "Commissioning complete");
        isCommissioningComplete = true;
        start_box();
        break;

    case chip::DeviceLayer::DeviceEventType::kFailSafeTimerExpired:
        ESP_LOGI(TAG, "Commissioning failed, fail safe timer expired");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStarted:
        ESP_LOGI(TAG, "Commissioning session started");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningSessionStopped:
        ESP_LOGI(TAG, "Commissioning session stopped");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowOpened:
        ESP_LOGI(TAG, "Commissioning window opened");
        break;

    case chip::DeviceLayer::DeviceEventType::kCommissioningWindowClosed:
        ESP_LOGI(TAG, "Commissioning window closed");
        if (isCommissioningComplete) {
            start_box();
        }
        break;

    default:
        break;
    }
}

static esp_err_t app_identification_cb(identification::callback_type_t type, uint16_t endpoint_id, uint8_t effect_id,
                                       uint8_t effect_variant, void *priv_data)
{
    ESP_LOGI(TAG, "Identification callback: type: %u, effect: %u, variant: %u", type, effect_id, effect_variant);
    return ESP_OK;
}

static esp_err_t app_attribute_update_cb(attribute::callback_type_t type, uint16_t endpoint_id, uint32_t cluster_id,
        uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
    esp_err_t err = ESP_OK;

    if (type == PRE_UPDATE) {
        /* Driver update */
    }

    return err;
}

extern "C" void app_main()
{
    esp_err_t err = ESP_OK;

    /* Initialize the ESP NVS layer */
    nvs_flash_init();

    /* Initialize driver */
    app_driver_handle_t switch_handle = matter_app_driver_switch_init();
    app_reset_button_register(switch_handle);

    /* Create a Matter node and add the mandatory Root Node device type on endpoint 0 */
    node::config_t node_config;
    node_t *node = node::create(&node_config, app_attribute_update_cb, app_identification_cb);

    on_off_switch::config_t switch_config;
    endpoint_t *endpoint_light = on_off_switch::create(node, &switch_config, ENDPOINT_FLAG_NONE, switch_handle);

    /* These node and endpoint handles can be used to create/add other endpoints and clusters. */
    if (!node || !endpoint_light) {
        ESP_LOGE(TAG, "Matter node creation failed");
    }

    /* Add group cluster to the switch endpoint */
    cluster::groups::config_t groups_config;
    cluster::groups::create(endpoint_light, &groups_config, CLUSTER_FLAG_SERVER | CLUSTER_FLAG_CLIENT);

    switch_endpoint_id = endpoint::get_id(endpoint_light);
    ESP_LOGI(TAG, "Switch created with endpoint_id %d", switch_endpoint_id);

    on_off_switch::config_t switch_config_2;
    endpoint_t *endpoint_fan = on_off_switch::create(node, &switch_config_2, ENDPOINT_FLAG_NONE, switch_handle);

    /* These node and endpoint handles can be used to create/add other endpoints and clusters. */
    if (!node || !endpoint_fan) {
        ESP_LOGE(TAG, "Matter node creation failed");
    }

    /* Add group cluster to the switch endpoint */
    cluster::groups::create(endpoint_fan, &groups_config, CLUSTER_FLAG_SERVER | CLUSTER_FLAG_CLIENT);

    switch_endpoint_id = endpoint::get_id(endpoint_fan);
    ESP_LOGI(TAG, "Switch created with endpoint_id %d", switch_endpoint_id);

    on_off_switch::config_t switch_config_3;
    endpoint_t *endpoint_switch = on_off_switch::create(node, &switch_config_3, ENDPOINT_FLAG_NONE, switch_handle);

    /* These node and endpoint handles can be used to create/add other endpoints and clusters. */
    if (!node || !endpoint_switch) {
        ESP_LOGE(TAG, "Matter node creation failed");
    }

    /* Add group cluster to the switch endpoint */
    cluster::groups::create(endpoint_switch, &groups_config, CLUSTER_FLAG_SERVER | CLUSTER_FLAG_CLIENT);

    switch_endpoint_id = endpoint::get_id(endpoint_switch);
    ESP_LOGI(TAG, "Switch created with endpoint_id %d", switch_endpoint_id);

    /* Matter start */
    err = esp_matter::start(app_event_cb);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Matter start failed: %d", err);
    }

#if CONFIG_ENABLE_CHIP_SHELL
    esp_matter::console::diagnostics_register_commands();
    esp_matter::console::init();
#endif
    box_sem = xSemaphoreCreateBinary();

    if (chip::Server::GetInstance().GetFabricTable().FabricCount() <= 0) {
        printf("Fabric Count is zero\n");
        xSemaphoreTake(box_sem, portMAX_DELAY);
        xSemaphoreTake(box_sem, portMAX_DELAY);
        vSemaphoreDelete(box_sem);
        box_sem = NULL;
    }

    box_main();
}
