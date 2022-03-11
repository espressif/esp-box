# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

if(DEFINED ENV{RMAKER_PATH})
  set(RMAKER_PATH $ENV{RMAKER_PATH})
else()
  set(RMAKER_PATH ${CMAKE_CURRENT_LIST_DIR}/../../components/esp-rainmaker)
  message("-- Not find RMAKER_PATH, default is ${RMAKER_PATH}")
endif(DEFINED ENV{RMAKER_PATH})

set(EXTRA_COMPONENT_DIRS
    ../../components
    $ENV{IDF_PATH}/examples/common_components/led_strip
    ${RMAKER_PATH}/components
    )

add_compile_options(-fdiagnostics-color=always)

project(factory_demo)
