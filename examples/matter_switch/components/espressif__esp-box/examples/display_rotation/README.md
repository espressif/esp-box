# BSP: Display Rotation Example

This example demonstrates usage of ESP-BOX Board Support Package. This is a single purpose example, which is focused on rotating LCD display: user can rotating display by buttons.

## How to use the example

### Hardware Required

* ESP-BOX
* USB-C Cable

### Compile and flash

```
idf.py -p COMx flash monitor
```

### Example outputs

```
I (241) cpu_start: ESP-IDF:          v5.0-dev-3434-g75b80d7a23
I (247) heap_init: Initializing. RAM available for dynamic allocation:
I (255) heap_init: At 3FC975C0 len 00048A40 (290 KiB): D/IRAM
I (261) heap_init: At 3FCE0000 len 0000EE34 (59 KiB): STACK/DRAM
I (268) heap_init: At 3FCF0000 len 00008000 (32 KiB): DRAM
I (274) heap_init: At 600FE000 len 00002000 (8 KiB): RTCRAM
I (281) spi_flash: detected chip: gd
I (284) spi_flash: flash io: dio
I (289) sleep: Configure to isolate all GPIO pins in sleep state
I (295) sleep: Enable automatic switching of GPIO sleep configuration
I (303) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.
I (325) gpio: GPIO[4]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (325) gpio: GPIO[48]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (455) ESP-BOX: Setting LCD backlight: 100%
I (455) ESP-BOX: Starting LVGL task
I (495) ESP-BOX: Example initialization done.
```
