if(CONFIG_ESP32_WROVER_KIT_BOARD)
    list(APPEND BSP_BOARD_SRC "./boards/esp32_wrover_kit.c")
endif()

if(CONFIG_ESP32_S2_KALUGA_BOARD)
    list(APPEND BSP_BOARD_SRC "./boards/esp32_s2_kaluga.c")
endif()

if(CONFIG_ESP32_S2_HMI_DEVKIT_BOARD)
    list(APPEND BSP_BOARD_SRC "./boards/esp32_s2_hmi_devkit.c")
endif()

if(CONFIG_ESP32_S3_HMI_DEVKIT_BOARD)
    list(APPEND BSP_BOARD_SRC "./boards/esp32_s3_hmi_devkit.c")
endif()

if(CONFIG_ESP32_S3_LCD_CAM_BOARD)
    list(APPEND BSP_BOARD_SRC "./boards/esp32_s3_lcd_cam.c")
endif()

if(CONFIG_ESP32_S3_CUBE_BOARD)
list(APPEND BSP_BOARD_SRC "./boards/esp32_s3_cube.c")
endif()

if(CONFIG_ESP32_C3_DEVKIT_BOARD)
    list(APPEND BSP_BOARD_SRC "./boards/esp32_c3_devkit.c")
endif()

file(GLOB_RECURSE BSP_SRCS
    "src/lcd/bsp_lcd.c"
    "src/lcd/bsp_lcd_spi.c"
    "src/lcd/bsp_lcd_i80.c"
    "src/bsp_codec.c"
    "src/bsp_i2c.c"
    "src/bsp_i2s.c"
    "src/bsp_storage.c"
    "src/bsp_tp.c")

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
