# CMakeLists.txt
#
# Copyright 2021 Espressif Systems (Shanghai) Co. Ltd.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#              http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#

# Search for src files
file(GLOB_RECURSE LVGL_SOURCES
    lvgl/src/*.c
    lvgl/examples/*.c)

file(GLOB_RECURSE LV_DEMOS_SOURCES lv_demos/src/*.c)

file(GLOB_RECURSE LV_LIB_SOURCES
    lv_lib/lv_lib_png/*.c
    lv_lib/lv_lib_bmp/*.c
    lv_lib/lv_lib_gif/*.c
    lv_lib/lv_lib_split_jpg/tjpgd.c
    lv_lib/lv_lib_split_jpg/lv_sjpg.c)

file(GLOB_RECURSE LV_PORT_SOURCES lv_port/*.c)

if (CONFIG_LV_FONT_USE_FREETYPE)
    list(APPEND LV_LIB_SOURCES lv_lib/lv_lib_freetype/lv_freetype.c)
endif()


# Resister components
idf_component_register(
    SRCS
        ${LVGL_SOURCES}
        ${LV_DEMOS_SOURCES}
        ${LV_LIB_SOURCES}
        ${LV_PORT_SOURCES}
    INCLUDE_DIRS
        "."
        "include"
        "lvgl/src"
        "lv_demos"
        "lv_lib"
        "lv_lib/lv_lib_bmp"
        "lv_lib/lv_lib_png"
        "lv_lib/lv_lib_gif"
        "lv_lib/lv_lib_split_jpg"
        "lv_lib/lv_lib_freetype"
        "lv_port"
    REQUIRES
        bsp
        main)

# Some of warinings from LVGL. Block them.
target_compile_options(${COMPONENT_LIB} PRIVATE -w)

# Use simple include
target_compile_definitions(${COMPONENT_LIB} PRIVATE "-DLV_CONF_INCLUDE_SIMPLE")
target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLV_DEMO_CONF_INCLUDE_SIMPLE")

# Required for `lv_lib_png`
target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLV_PNG_USE_LV_FILESYSTEM")

if (CONFIG_LV_MEM_CUSTOM)
    if (CONFIG_LV_MEM_CUSTOM_ALLOC)
        target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLV_MEM_CUSTOM_ALLOC=${CONFIG_LV_MEM_CUSTOM_ALLOC}")
    endif()

    if (CONFIG_LV_MEM_CUSTOM_FREE)
        target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLV_MEM_CUSTOM_FREE=${CONFIG_LV_MEM_CUSTOM_FREE}")
    endif()
endif()

if (CONFIG_LV_TICK_CUSTOM)
    if (CONFIG_LV_TICK_CUSTOM_SYS_TIME_EXPR)
        target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLV_TICK_CUSTOM_SYS_TIME_EXPR=${CONFIG_LV_TICK_CUSTOM_SYS_TIME_EXPR}")
    endif()
endif()

if (CONFIG_LV_USER_DATA_FREE)
    target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLV_USER_DATA_FREE=${CONFIG_LV_USER_DATA_FREE}")
endif()

if (CONFIG_LV_ATTRIBUTE_FAST_MEM_USE_IRAM)
    target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLV_ATTRIBUTE_FAST_MEM=IRAM_ATTR")
endif()
