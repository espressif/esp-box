


idf_component_register(
    SRC_DIRS
        "src/boards"
        "src/peripherals"
        "src/storage"
        "src/codec/es7210"
        "src/codec/es7243e"
        "src/codec/es8156"
        "src/codec/es8311"
        "src/codec/es8388" 
    INCLUDE_DIRS
        "src/boards"
        "include"
        "src/codec/es7210"
        "src/codec/es7243e"
        "src/codec/es8156"
        "src/codec/es8311"
        "src/codec/es8388"
        "src/codec/include"
    REQUIRES
        iot_button
        esp_hid
        esp_lcd
        fatfs
        i2c_bus
        i2c_devices
        spiffs)
