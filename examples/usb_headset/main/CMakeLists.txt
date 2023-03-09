idf_component_register( SRC_DIRS "src" "."
                        EXCLUDE_SRCS "src/usb_descriptors.c"
                        INCLUDE_DIRS include/.)
idf_component_get_property(tusb_lib espressif__tinyusb COMPONENT_LIB)
idf_component_get_property(tusb_path espressif__tinyusb COMPONENT_DIR)
target_include_directories(${tusb_lib} PRIVATE include)
set_source_files_properties("${tusb_path}/src/portable/espressif/esp32sx/dcd_esp32sx.c" PROPERTIES EXCLUDE_FROM_ALL TRUE TARGET ${tusb_lib})
target_sources(${tusb_lib} PUBLIC "src/usb_descriptors.c" "${tusb_path}/src/portable/synopsys/dwc2/dcd_dwc2.c")