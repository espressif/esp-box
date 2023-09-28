string(REGEX MATCH "factory_demo" PROJECT_IS_FACTORY_DEMO "${PROJECT_DIR}")

if(EXISTS ${PROJECT_DIR}/sdkconfig)
    file(READ ${PROJECT_DIR}/sdkconfig SDKCONFIG_RULE)
    string(REGEX MATCH "CONFIG_BSP_BOARD_ESP32_S3_BOX_Lite=y" COMPILER_TARGET_IS_BOX_LITE "${SDKCONFIG_RULE}")
    string(REGEX MATCH "CONFIG_BSP_BOARD_ESP32_S3_BOX=y" COMPILER_TARGET_IS_ESP_BOX "${SDKCONFIG_RULE}")
    string(REGEX MATCH "CONFIG_BSP_BOARD_ESP32_S3_BOX_3=y" COMPILER_TARGET_IS_ESP_BOX_3 "${SDKCONFIG_RULE}")
else()
    message(STATUS "sdkconfig not exist")
    file(READ ${CMAKE_CURRENT_LIST_DIR}/Kconfig.projbuild SDKCONFIG_RULE)
    string(REGEX MATCH "default BSP_BOARD_ESP32_S3_BOX_Lite" COMPILER_TARGET_IS_BOX_LITE "${SDKCONFIG_RULE}")
    string(REGEX MATCH "default BSP_BOARD_ESP32_S3_BOX" COMPILER_TARGET_IS_ESP_BOX "${SDKCONFIG_RULE}")
    string(REGEX MATCH "default BSP_BOARD_ESP32_S3_BOX_3" COMPILER_TARGET_IS_ESP_BOX_3 "${SDKCONFIG_RULE}")
endif()

if (COMPILER_TARGET_IS_ESP_BOX_3)
    message(STATUS "PLATFORM ESP32_S3_BOX_3.")
    set(box_alias "-3")
elseif (COMPILER_TARGET_IS_BOX_LITE)
    message(STATUS "PLATFORM ESP32_S3_BOX_Lite.")
    set(box_alias "-lite")
    set(bsp_src "src/storage/bsp_sdcard.c")
elseif (COMPILER_TARGET_IS_ESP_BOX)
    message(STATUS "PLATFORM ESP32_S3_BOX.")
    set(bsp_src "src/storage/bsp_sdcard.c")
else()
    message(FATAL_ERROR "PLATFORM unknown.")
endif()

set(requires "driver" "fatfs")
set(priv_requires "esp-box${box_alias}")

if (PROJECT_IS_FACTORY_DEMO AND COMPILER_TARGET_IS_ESP_BOX_3)
    list(APPEND priv_requires "aht20" "at581x")
    list(APPEND bsp_src "src/boards/esp32_bsp_sensor.c")
else()
    list(APPEND bsp_src "src/boards/esp32_bsp_no_sensor.c")
endif()

list(APPEND bsp_src "src/boards/esp32_bsp_board.c")

idf_component_register(
    SRCS ${bsp_src}
    INCLUDE_DIRS "src/boards" "include"
    PRIV_INCLUDE_DIRS "priv_include"
    PRIV_REQUIRES ${priv_requires}
    REQUIRES ${requires})
