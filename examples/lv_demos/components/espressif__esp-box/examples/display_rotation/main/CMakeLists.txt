file(GLOB_RECURSE IMAGE_SOURCES images/*.c)

idf_component_register(SRCS "main.c" ${IMAGE_SOURCES}
                    INCLUDE_DIRS ".")
