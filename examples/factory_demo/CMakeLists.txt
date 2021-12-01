cmake_minimum_required(VERSION 3.5)

set(PROJECT_VER "0.2.1")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS
    $ENV{IDF_PATH}/examples/common_components/protocol_examples_common
    $ENV{IDF_PATH}/examples/common_components/led_strip
    ../../components)

add_compile_options(-fdiagnostics-color=always)

project(factory_demo)
