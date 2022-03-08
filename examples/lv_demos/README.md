# Hello World Example

| Board             | Support Status |
| ----------------- | -------------- |
| ESP32-S3-BOX      | YES            |
| ESP32-S3-BOX-Lite | NO             |

LVGL official demos.

## How to use example

### Hardware Required

* A ESP32-S3-Box
* An USB-C cable for power supply and programming

### Build and Flash

Run `idf.py flash monitor` to build, flash and monitor the project.

Once a complete flash process has been performed, you can use `idf.py app-flash monitor` to reduce the flash time.

(To exit the serial monitor, type `Ctrl-]`. Please reset the development board f you cannot exit the monitor.)

## Example Output

Run this example, you will see the following output log:

```
SPIWP:0xee
mode:DIO, clock div:1
load:0x3fcd0108,len:0x18c0
load:0x403b6000,len:0xb58
load:0x403ba000,len:0x2fdc
entry 0x403b61e4
I (839) boot: ESP-IDF v5.0-dev-19-gfe2c978b33-dirty 2nd stage bootloader
I (839) boot: compile time 16:08:17
I (839) boot: chip revision: 0
I (843) qio_mode: Enabling default flash chip QIO
I (848) boot.esp32s3: Boot SPI Speed : 80MHz
I (853) boot.esp32s3: SPI Mode       : QIO
I (858) boot.esp32s3: SPI Flash Size : 16MB
I (863) boot: Enabling RNG early entropy source...
W (868) bootloader_random: RNG for ESP32-S3 not currently supported
I (875) boot: Partition Table:
I (879) boot: ## Label            Usage          Type ST Offset   Length
I (886) boot:  0 nvs              WiFi data        01 02 00009000 00006000
I (894) boot:  1 phy_init         RF data          01 01 0000f000 00001000
I (901) boot:  2 factory          factory app      00 00 00010000 00100000
I (909) boot: End of partition table
I (913) esp_image: segment 0: paddr=00010020 vaddr=3c050020 size=2c290h (180880) map
I (949) esp_image: segment 1: paddr=0003c2b8 vaddr=3fc96ff0 size=02a08h ( 10760) load
I (952) esp_image: segment 2: paddr=0003ecc8 vaddr=40378000 size=01350h (  4944) load
I (956) esp_image: segment 3: paddr=00040020 vaddr=42000020 size=4961ch (300572) map
I (1009) esp_image: segment 4: paddr=00089644 vaddr=40379350 size=0dc9ch ( 56476) load
I (1020) esp_image: segment 5: paddr=000972e8 vaddr=50000000 size=00010h (    16) load
I (1027) boot: Loaded app from partition at offset 0x10000
I (1027) boot: Disabling RNG early entropy source...
W (1030) bootloader_random: RNG for ESP32-S3 not currently supported
I (1037) cpu_start: Pro cpu up.
I (1040) cpu_start: Starting app cpu, entry point is 0x40379264
0x40379264: call_start_cpu1 at /home/lijiaiq/esp/esp-idf-wifi-ok/components/esp_system/port/cpu_start.c:156

I (0) cpu_start: App cpu up.
I (1057) cpu_start: Pro cpu start user code
I (1057) cpu_start: cpu freq: 240000000
I (1057) cpu_start: Application information:
I (1057) cpu_start: Project name:     lv_demos
I (1057) cpu_start: App version:      f92fece2-dirty
I (1057) cpu_start: Compile time:     Nov 19 2021 16:08:02
I (1058) cpu_start: ELF file SHA256:  287b2378d4b92caa...
I (1058) cpu_start: ESP-IDF:          v5.0-dev-19-gfe2c978b33-dirty
I (1058) heap_init: Initializing. RAM available for dynamic allocation:
I (1058) heap_init: At 3FCA10A8 len 0003EF58 (251 KiB): D/IRAM
I (1059) heap_init: At 3FCE0000 len 0000EE34 (59 KiB): STACK/DRAM
I (1059) heap_init: At 600FE000 len 00002000 (8 KiB): RTCRAM
I (1060) spi_flash: detected chip: gd
I (1060) spi_flash: flash io: qio
I (1060) sleep: Configure to isolate all GPIO pins in sleep state
I (1061) sleep: Enable automatic switching of GPIO sleep configuration
I (1061) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.
I (1062) gpio: GPIO[1]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:3 
I (1062) gpio: GPIO[4]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (1062) gpio: GPIO[48]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (1182) gpio: GPIO[45]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (1182) bsp_tp: Detected touch panel at 0x24. Vendor : Parade Tech
I (1222) gpio: GPIO[3]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0 
```
