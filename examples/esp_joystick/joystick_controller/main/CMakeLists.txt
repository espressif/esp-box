#Add sources from ui directory
file(GLOB_RECURSE SRC_UI ${CMAKE_SOURCE_DIR} "ui/*.c" "app/*.c")

idf_component_register(SRCS "joystick_controller_main.c" "app_ui_event.c" ${SRC_UI}
                    INCLUDE_DIRS "." "ui" "app")

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable)
