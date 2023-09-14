file(GLOB_RECURSE LV_DEMOS_SOURCES ./*.c)

idf_component_register(SRC_DIRS          "." "./app" "./gui" "./gui/font" "./gui/image" "./rmaker"
                      PRIV_INCLUDE_DIRS  "." "./app" "./gui" "./rmaker")

set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 14)
target_compile_options(${COMPONENT_LIB} PRIVATE "-DLWIP_IPV6_SCOPES=0" "-DCHIP_HAVE_CONFIG_H")

set_source_files_properties(
    ${LV_DEMOS_SOURCES}
    PROPERTIES COMPILE_OPTIONS
    -DLV_LVGL_H_INCLUDE_SIMPLE)

spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)
set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)