#Add sources from ui directory
file(GLOB_RECURSE SRC_UI ${CMAKE_SOURCE_DIR} "espnow_ctrl/*.c")

idf_component_register(SRCS "joystick_rc_receiver_main.c" ${SRC_UI}
                       INCLUDE_DIRS "." "espnow_ctrl")

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable)
