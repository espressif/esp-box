set(UI_DIR ./ui)
file(GLOB_RECURSE UI_SRCS ${UI_DIR}/*.c)

set(APP_DIR ./app)
file(GLOB_RECURSE APP_SRCS ${APP_DIR}/*.c)

set(SETTINGS_DIR ./settings)
file(GLOB_RECURSE SETTINGS_SRCS ${SETTINGS_DIR}/*.c)

idf_component_register(
    SRCS
        "main.c"
        ${UI_SRCS}
        ${APP_SRCS}
        ${SETTINGS_SRCS}
    INCLUDE_DIRS
        "."
        ${UI_DIR}
        ${APP_DIR}
        ${SETTINGS_DIR}
)

set(MV_UF2_BIN_EXE ${COMPONENT_PATH}/generateuf2.py)
idf_build_get_property(build_dir BUILD_DIR)
set(nvs_src_file ${PROJECT_DIR}/factory_nvs/build/factory_nvs.bin)
set(nvs_dst_file ${build_dir}/uf2/factory_nvs.bin)

if(NOT EXISTS ${nvs_src_file})
    message(FATAL_ERROR "Please ensure that the 'factory_nvs' is built first, and make sure to keep the 'HMI Board Config' consistent.")
endif()

add_custom_command(
        OUTPUT ${nvs_dst_file}
        COMMENT "Generate factory_nvs..."
        COMMAND python ${MV_UF2_BIN_EXE} -d1 ${nvs_src_file} -d2 ${nvs_dst_file}
        VERBATIM)

add_custom_target(nvs_uf2_bin ALL DEPENDS ${nvs_dst_file})
add_dependencies(flash nvs_uf2_bin)

partition_table_get_partition_info(size "--partition-name ota_0" "size")
partition_table_get_partition_info(offset "--partition-name ota_0" "offset")

if("${size}" AND "${offset}")
    esptool_py_flash_to_partition(flash "ota_0" "${nvs_dst_file}")
else()
    set(message "Failed to find ota_0 in partition table file"
    "Please add a line to the partition file.")
endif()

spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)
