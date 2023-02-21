file(GLOB_RECURSE LV_DEMOS_SOURCES ./*.c)

idf_component_register(
    SRC_DIRS
        "."
        "app"
        "gui"
        "gui/font"
        "gui/image"
        "rmaker"

    INCLUDE_DIRS
        "."
        "gui"
        "app"
        "rmaker")

target_compile_definitions(${COMPONENT_TARGET} PRIVATE "-D RMAKER_DEMO_PROJECT_NAME=\"${CMAKE_PROJECT_NAME}\"")

set_source_files_properties(
    ${LV_DEMOS_SOURCES}
    PROPERTIES COMPILE_OPTIONS
    -DLV_LVGL_H_INCLUDE_SIMPLE)

spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)
