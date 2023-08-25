# LVGL ESP Portation

[![Component Registry](https://components.espressif.com/components/espressif/esp_lvgl_port/badge.svg)](https://components.espressif.com/components/espressif/esp_lvgl_port)

This component helps with using LVGL with Espressif's LCD and touch drivers. It can be used with any project with LCD display. 

## Features
* Initialization of the LVGL
    * Create task and timer
    * Handle rotating
* Add/remove display (using [`esp_lcd`](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/lcd.html))
* Add/remove touch input (using [`esp_lcd_touch`](https://github.com/espressif/esp-bsp/tree/master/components/lcd_touch))
* Add/remove navigation buttons input (using [`button`](https://github.com/espressif/esp-iot-solution/tree/master/components/button))
* Add/remove encoder input (using [`knob`](https://github.com/espressif/esp-iot-solution/tree/master/components/knob))

## Usage

### Initialization
``` c
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    esp_err_t err = lvgl_port_init(&lvgl_cfg);
```

### Add screen

Add an LCD screen to the LVGL. It can be called multiple times for adding multiple LCD screens. 

``` c
    static lv_disp_t * disp_handle;
    
    /* LCD IO */
	esp_lcd_panel_io_handle_t io_handle = NULL;
	ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t) 1, &io_config, &io_handle));

    /* LCD driver initialization */ 
    esp_lcd_panel_handle_t lcd_panel_handle;
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &lcd_panel_handle));

    /* Add LCD screen */
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = lcd_panel_handle,
        .buffer_size = DISP_WIDTH*DISP_HEIGHT,
        .double_buffer = true,
        .hres = DISP_WIDTH,
        .vres = DISP_HEIGHT,
        .monochrome = false,
        /* Rotation values must be same as used in esp_lcd for initial settings of the screen */
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
        }
    };
    disp_handle = lvgl_port_add_disp(&disp_cfg);
    
    /* ... the rest of the initialization ... */

    /* If deinitializing LVGL port, remember to delete all displays: */
    lvgl_port_remove_disp(disp_handle);
```

### Add touch input

Add touch input to the LVGL. It can be called more times for adding more touch inputs. 
``` c
    /* Touch driver initialization */
    ...
    esp_lcd_touch_handle_t tp;
    esp_err_t err = esp_lcd_touch_new_i2c_gt911(io_handle, &tp_cfg, &tp);

    /* Add touch input (for selected screen) */
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = disp_handle,
        .handle = tp,
    };
    lv_indev_t* touch_handle = lvgl_port_add_touch(&touch_cfg);
    
    /* ... the rest of the initialization ... */

    /* If deinitializing LVGL port, remember to delete all touches: */
    lvgl_port_remove_touch(touch_handle);
```

### Add buttons input

Add buttons input to the LVGL. It can be called more times for adding more buttons inputs for different displays. This feature is available only when the component `espressif/button` was added into the project.
``` c
    /* Buttons configuration structure */
    const button_config_t bsp_button_config[] = {
        {
            .type = BUTTON_TYPE_ADC,
            .adc_button_config.adc_channel = ADC_CHANNEL_0, // ADC1 channel 0 is GPIO1
            .adc_button_config.button_index = 0,
            .adc_button_config.min = 2310, // middle is 2410mV
            .adc_button_config.max = 2510
        },
        {
            .type = BUTTON_TYPE_ADC,
            .adc_button_config.adc_channel = ADC_CHANNEL_0, // ADC1 channel 0 is GPIO1
            .adc_button_config.button_index = 1,
            .adc_button_config.min = 1880, // middle is 1980mV
            .adc_button_config.max = 2080
        },
        {
            .type = BUTTON_TYPE_ADC,
            .adc_button_config.adc_channel = ADC_CHANNEL_0, // ADC1 channel 0 is GPIO1
            .adc_button_config.button_index = 2,
            .adc_button_config.min = 720, // middle is 820mV
            .adc_button_config.max = 920
        },
    };

    const lvgl_port_nav_btns_cfg_t btns = {
        .disp = disp_handle,
        .button_prev = &bsp_button_config[0],
        .button_next = &bsp_button_config[1],
        .button_enter = &bsp_button_config[2]
    };

    /* Add buttons input (for selected screen) */
    lv_indev_t* buttons_handle = lvgl_port_add_navigation_buttons(&btns);
    
    /* ... the rest of the initialization ... */

    /* If deinitializing LVGL port, remember to delete all buttons: */
    lvgl_port_remove_navigation_buttons(buttons_handle);
```

**Note:** When you use navigation buttons for control LVGL objects, these objects must be added to LVGL groups. See [LVGL documentation](https://docs.lvgl.io/master/overview/indev.html?highlight=lv_indev_get_act#keypad-and-encoder) for more info.

### Add encoder input

Add encoder input to the LVGL. It can be called more times for adding more encoder inputs for different displays. This feature is available only when the component `espressif/knob` was added into the project.
``` c

    const button_config_t encoder_btn_config = {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config.active_level = false,
        .gpio_button_config.gpio_num = GPIO_BTN_PRESS,
    };

    const knob_config_t encoder_a_b_config = {
        .default_direction = 0,
        .gpio_encoder_a = GPIO_ENCODER_A,
        .gpio_encoder_b = GPIO_ENCODER_B,
    };

    /* Encoder configuration structure */
    const lvgl_port_encoder_cfg_t encoder = {
        .disp = disp_handle,
        .encoder_a_b = &encoder_a_b_config,
        .encoder_enter = &encoder_btn_config
    };

    /* Add encoder input (for selected screen) */
    lv_indev_t* encoder_handle = lvgl_port_add_encoder(&encoder);
    
    /* ... the rest of the initialization ... */

    /* If deinitializing LVGL port, remember to delete all encoders: */
    lvgl_port_remove_encoder(encoder_handle);
```

**Note:** When you use encoder for control LVGL objects, these objects must be added to LVGL groups. See [LVGL documentation](https://docs.lvgl.io/master/overview/indev.html?highlight=lv_indev_get_act#keypad-and-encoder) for more info.

### LVGL API usage

Every LVGL calls must be protected with these lock/unlock commands:
``` c
	/* Wait for the other task done the screen operation */
    lvgl_port_lock(0);
    ...
    lv_obj_t * screen = lv_disp_get_scr_act(disp_handle);
    lv_obj_t * obj = lv_label_create(screen);
    ...
    /* Screen operation done -> release for the other task */
    lvgl_port_unlock();
```

### Rotating screen
``` c
    lv_disp_set_rotation(disp_handle, LV_DISP_ROT_90);
```

**Note:** During the rotating, the component call [`esp_lcd`](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/lcd.html) API.

## Performance

Key feature of every graphical application is performance. Recommended settings for improving LCD performance is described in a separate document [here](docs/performance.md).
