set(component_srcs "src/esp_schedule.c"
                   "src/esp_schedule_nvs.c")

idf_component_register(SRCS "${component_srcs}"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS "src"
                       PRIV_REQUIRES "rmaker_common"
                       REQUIRES "nvs_flash")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-function")
