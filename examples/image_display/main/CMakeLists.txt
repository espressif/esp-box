idf_component_register(
    SRCS
        "image_display.c"
    INCLUDE_DIRS
        "")

spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)
