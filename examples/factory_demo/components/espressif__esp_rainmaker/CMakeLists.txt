# CORE
set(core_srcs "src/core/esp_rmaker_core.c"
        "src/core/esp_rmaker_node.c"
        "src/core/esp_rmaker_device.c"
        "src/core/esp_rmaker_param.c"
        "src/core/esp_rmaker_node_config.c"
        "src/core/esp_rmaker_client_data.c"
        "src/core/esp_rmaker_time_service.c"
        "src/core/esp_rmaker_system_service.c"
        "src/core/esp_rmaker_user_mapping.pb-c.c"
        "src/core/esp_rmaker_user_mapping.c"
        "src/core/esp_rmaker_schedule.c"
        "src/core/esp_rmaker_scenes.c"
        "src/core/esp_rmaker_cmd_resp_manager.c"
        )

set(priv_req protobuf-c json_parser json_generator wifi_provisioning
             nvs_flash esp_http_client app_update esp-tls mbedtls esp_https_ota
             console esp_local_ctrl esp_https_server mdns esp_schedule efuse driver rmaker_common)

if(CONFIG_ESP_RMAKER_ASSISTED_CLAIM)
    list(APPEND core_srcs
        "src/core/esp_rmaker_claim.c"
        "src/core/esp_rmaker_claim.pb-c.c")
endif()
if(CONFIG_ESP_RMAKER_SELF_CLAIM)
    list(APPEND core_srcs
        "src/core/esp_rmaker_claim.c")
endif()

if(CONFIG_ESP_RMAKER_LOCAL_CTRL_ENABLE)
    list(APPEND core_srcs
        "src/core/esp_rmaker_local_ctrl.c")
endif()

set(core_priv_includes "src/core")

# MQTT
set(mqtt_srcs "src/mqtt/esp_rmaker_mqtt.c"
        "src/mqtt/esp_rmaker_mqtt_budget.c")
set(mqtt_priv_includes "src/mqtt")

# OTA
set(ota_srcs "src/ota/esp_rmaker_ota.c"
        "src/ota/esp_rmaker_ota_using_params.c"
        "src/ota/esp_rmaker_ota_using_topics.c")
set(ota_priv_includes "src/ota")

# CONSOLE
set(console_srcs "src/console/esp_rmaker_console.c"
        "src/console/esp_rmaker_commands.c")
set(console_priv_includes "src/console")

# STANDARD TYPES
set(standard_types_srcs "src/standard_types/esp_rmaker_standard_params.c"
        "src/standard_types/esp_rmaker_standard_devices.c"
        "src/standard_types/esp_rmaker_standard_services.c")

idf_component_register(SRCS ${core_srcs} ${mqtt_srcs} ${ota_srcs} ${standard_types_srcs} ${console_srcs}
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS ${core_priv_includes} ${ota_priv_includes} ${console_priv_includes} ${mqtt_priv_includes}
                       REQUIRES rmaker_common
                       PRIV_REQUIRES ${priv_req})

target_add_binary_data(${COMPONENT_TARGET} "server_certs/rmaker_mqtt_server.crt" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "server_certs/rmaker_claim_service_server.crt" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "server_certs/rmaker_ota_server.crt" TEXT)

# Added just to automatically trigger re-runs of CMake
git_describe(RMAKER_VERSION ${COMPONENT_DIR})
message("ESP RainMaker Project commit: " ${RMAKER_VERSION})
