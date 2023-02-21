set(LV_DEMO_DIR ../managed_components/lvgl__lvgl/demos)
file(GLOB_RECURSE LV_DEMOS_SOURCES ${LV_DEMO_DIR}/*.c ./*.c)

idf_component_register(
    SRCS
        "lv_demos.c"
        ${LV_DEMOS_SOURCES})

set_source_files_properties(
            ${LV_DEMOS_SOURCES}
            PROPERTIES COMPILE_OPTIONS
            -DLV_LVGL_H_INCLUDE_SIMPLE)