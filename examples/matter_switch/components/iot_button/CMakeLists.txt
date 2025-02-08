include($ENV{ESP_MATTER_DEVICE_PATH}/esp_matter_device.cmake)

set(requires driver button)

idf_component_register(SRC_DIRS
                    INCLUDE_DIRS
                    REQUIRES ${requires})
