#Add sources from ui directory
file(GLOB_RECURSE SRC_UI ${CMAKE_SOURCE_DIR} "espnow_ctrl/*.c")

idf_component_register(SRCS "joystick_rc_receiver_main.c" ${SRC_UI}
                       INCLUDE_DIRS "." "espnow_ctrl")

idf_component_get_property(lib espressif__esp-now COMPONENT_LIB)
target_compile_options(${lib} PRIVATE -Wno-implicit-function-declaration)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable)
