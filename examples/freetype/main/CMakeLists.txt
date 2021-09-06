idf_component_register(
    SRCS
        "freetype_example.c"
    INCLUDE_DIRS
        "lib/freetype/include")

# Build static library, do not build test executables
option(BUILD_SHARED_LIBS OFF)
option(BUILD_TESTING OFF)
option(SKIP_INSTALL_ALL ON)

# Unfortunately the library performs install and export. Would
# have been nice if devs made that an option like BUILD_SHARED_LIBS
# and BUILD_TESTING. Override install() and export() to do nothing
# instead.
function(install)
endfunction()

function(export)
endfunction()

# Import freetype targets
add_subdirectory(lib)

# Link freetype to main component
target_link_libraries(${COMPONENT_LIB} PUBLIC freetype)
    
spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)
