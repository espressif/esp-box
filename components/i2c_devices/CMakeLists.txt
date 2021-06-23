file(GLOB_RECURSE CSRC_FILES *.c)
idf_component_register(
    SRCS
        ${CSRC_FILES}
    INCLUDE_DIRS
        "include"
    REQUIRES
        bsp)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-unused-variable
    -Wno-unused-function
    -Wno-incompatible-pointer-types)
