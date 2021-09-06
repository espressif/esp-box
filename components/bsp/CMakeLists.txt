file(GLOB_RECURSE BSP_SRCS
    "src/lcd/bsp_lcd.c"
    "src/lcd/bsp_lcd_spi.c"
    "src/lcd/bsp_lcd_i80.c"
    "src/bsp_codec.c"
    "src/bsp_i2c.c"
    "src/bsp_i2s.c"
    "src/bsp_storage.c"
    "src/bsp_tp.c")

if(CONFIG_ESP32_S3_CUBE_BOARD)
    list(APPEND BSP_BOARD_SRC "./boards/esp32_s3_cube.c")
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
        esp_lcd
        i2c_bus
        i2c_devices
        lvgl)
