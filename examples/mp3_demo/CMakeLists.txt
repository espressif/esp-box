cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

add_compile_options(-fdiagnostics-color=always)

# The following line can be removed after LVGL update to v8.3.5.
add_compile_options(-Wno-format)

project(mp3_demo)

