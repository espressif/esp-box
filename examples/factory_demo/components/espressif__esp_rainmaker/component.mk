COMPONENT_SRCDIRS := src/core src/mqtt src/ota src/standard_types src/console
COMPONENT_ADD_INCLUDEDIRS := include
COMPONENT_PRIV_INCLUDEDIRS := src/core src/ota src/console

ifndef CONFIG_ESP_RMAKER_ASSISTED_CLAIM
COMPONENT_OBJEXCLUDE += src/core/esp_rmaker_claim.pb-c.o
ifndef CONFIG_ESP_RMAKER_SELF_CLAIM
    COMPONENT_OBJEXCLUDE += src/core/esp_rmaker_claim.o
endif
endif

ifndef CONFIG_ESP_RMAKER_LOCAL_CTRL_ENABLE
COMPONENT_OBJEXCLUDE += src/core/esp_rmaker_local_ctrl.o
endif

COMPONENT_EMBED_TXTFILES := server_certs/rmaker_mqtt_server.crt server_certs/rmaker_claim_service_server.crt server_certs/rmaker_ota_server.crt
