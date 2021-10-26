file(GLOB_RECURSE BSP_SRCS
    "src/bsp_codec.c"
    "src/bsp_i2c.c"
    "src/bsp_i2s.c"
    "src/bsp_tp.c"
    "src/lcd/*.c"
    "src/storage/*.c")

if(CONFIG_ESP32_S3_BOX_BOARD)
    list(APPEND BSP_BOARD_SRC "./boards/esp32_s3_box.c")
endif()

if(CONFIG_ESP_CUSTOM_BOARD)
    list(APPEND BSP_BOARD_SRC "./boards/esp_custom_board.c")
endif()

idf_component_register(
    SRCS
        ${BSP_SRCS}
        ${BSP_BOARD_SRC}
    INCLUDE_DIRS
        "boards"
        "include"
    REQUIRES
        codec
        i2c_devices
        esp_lcd
        fatfs
        spiffs)
