# if(CONFIG_ESP32_S3_CUBE_BOARD)

    set(COMPONENT_SRCDIRS 
        ./MediaHal
        ./MediaHal/Codec
        ./SystemSal
        ./button
        ./SDCardConfig
        ./led
        )

    set(COMPONENT_ADD_INCLUDEDIRS 
        ./MediaHal/Board
        ./MediaHal/Codec
        ./MediaHal
        ./SystemSal
        ./userconfig
        ./button/include
        ./SDCardConfig
        ./led/include
        )

    set(COMPONENT_REQUIRES
        fatfs
        esp_adc_cal
        nvs_flash
        )

register_component()

# endif()
