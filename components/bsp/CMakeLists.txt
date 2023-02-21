set(bsp_src "src/storage/bsp_sdcard.c")

if (CONFIG_BSP_BOARD_ESP32_S3_BOX)
    list(APPEND bsp_src "src/boards/esp32_s3_box.c")
elseif (CONFIG_BSP_BOARD_ESP32_S3_BOX_Lite)
    list(APPEND bsp_src "src/boards/esp32_s3_box_lite.c")
else()
    #message(FATAL_ERROR "Platform unknown.")
endif()

set(priv_requires "es7210" "esp-box")

set(requires "driver" "fatfs")

idf_component_register(
    SRCS ${bsp_src}
    INCLUDE_DIRS
        "src/boards"
        "include"
    PRIV_REQUIRES ${priv_requires}
    REQUIRES ${requires})
