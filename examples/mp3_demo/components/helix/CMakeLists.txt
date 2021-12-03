idf_component_register(
    SRC_DIRS
        "src"
    INCLUDE_DIRS
        "include")

# Some of warinings from LVGL. Block them.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-but-set-variable)
