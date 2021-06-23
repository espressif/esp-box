file(GLOB_RECURSE FONTS_SOURCES font/*.c)
file(GLOB_RECURSE IMAGE_SOURCES image/*.c)

idf_component_register(
    SRCS
        "app_sr.c"
        "demo_main.c"
        "sr_handler.c"
        "ui_main.c"
        ${FONTS_SOURCES}
        ${IMAGE_SOURCES}
    INCLUDE_DIRS
        "include")

# spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)
