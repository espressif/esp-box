file(GLOB_RECURSE APP_SRCS app/*.c)
file(GLOB_RECURSE FONT_SRCS font/*.c)
file(GLOB_RECURSE IMG_SRCS image/*.c)

idf_component_register(
    SRCS
        "demo_main.c"
        "sr_handler.c"
        "ui_main.c"
        ${APP_SRCS}
        ${FONT_SRCS}
        ${IMG_SRCS}
    INCLUDE_DIRS
        "include")

spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)
