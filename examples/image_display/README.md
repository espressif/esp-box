# Image Display Example

| Board             | Support Status |
| ----------------- | -------------- |
| ESP32-S3-BOX      | YES            |
| ESP32-S3-BOX-Lite | YES            |

Display PNG image with LVGL.

## How to use example

### Hardware Required

* A ESP32-S3-Box
* An USB-C cable for power supply and programming

Follow detailed instructions provided specifically for this example. 

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
I (839) boot: compile time 16:01:59
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
I (901) boot:  2 factory          factory app      00 00 00010000 00200000
I (909) boot:  3 storage          Unknown data     01 82 00210000 00100000
I (917) boot: End of partition table
I (921) esp_image: segment 0: paddr=00010020 vaddr=3c060020 size=11f60h ( 73568) map
I (941) esp_image: segment 1: paddr=00021f88 vaddr=3fc985d0 size=02c28h ( 11304) load
I (943) esp_image: segment 2: paddr=00024bb8 vaddr=40378000 size=0b460h ( 46176) load
I (955) esp_image: segment 3: paddr=00030020 vaddr=42000020 size=500c4h (327876) map
I (1005) esp_image: segment 4: paddr=000800ec vaddr=40383460 size=05168h ( 20840) load
I (1010) esp_image: segment 5: paddr=0008525c vaddr=50000000 size=00010h (    16) load
I (1018) boot: Loaded app from partition at offset 0x10000
I (1018) boot: Disabling RNG early entropy source...
W (1023) bootloader_random: RNG for ESP32-S3 not currently supported
I (1030) opi psram: vendor id : 0x0d (AP)
I (1035) opi psram: dev id    : 0x02 (generation 3)
I (1040) opi psram: density   : 0x03 (64 Mbit)
I (1045) opi psram: good-die  : 0x01 (Pass)
I (1050) opi psram: Latency   : 0x01 (Fixed)
I (1055) opi psram: VCC       : 0x01 (3V)
I (1060) opi psram: SRF       : 0x01 (Fast Refresh)
I (1066) opi psram: BurstType : 0x01 (Hybrid Wrap)
I (1071) opi psram: BurstLen  : 0x01 (32 Byte)
I (1076) opi psram: Readlatency  : 0x02 (10 cycles@Fixed)
I (1082) opi psram: DriveStrength: 0x00 (1/1)
W (1087) PSRAM: DO NOT USE FOR MASS PRODUCTION! Timing parameters will be updated in future IDF version.
I (1098) spiram: Found 64MBit SPI RAM device
I (1102) spiram: SPI RAM mode: sram 80m
I (1107) spiram: PSRAM initialized, cache is in normal (1-core) mode.
I (1114) cpu_start: Pro cpu up.
I (1118) cpu_start: Starting app cpu, entry point is 0x403793cc
0x403793cc: call_start_cpu1 at /home/lijiaiq/esp/esp-idf-wifi-ok/components/esp_system/port/cpu_start.c:156

I (0) cpu_start: App cpu up.
I (1544) spiram: SPI SRAM memory test OK
I (1553) cpu_start: Pro cpu start user code
I (1554) cpu_start: cpu freq: 240000000
I (1554) cpu_start: Application information:
I (1554) cpu_start: Project name:     image_display
I (1554) cpu_start: App version:      f92fece2-dirty
I (1554) cpu_start: Compile time:     Nov 19 2021 16:01:44
I (1555) cpu_start: ELF file SHA256:  b389f1d8f7c2d501...
I (1555) cpu_start: ESP-IDF:          v5.0-dev-19-gfe2c978b33-dirty
I (1555) heap_init: Initializing. RAM available for dynamic allocation:
I (1555) heap_init: At 3FCA2868 len 0003D798 (245 KiB): D/IRAM
I (1556) heap_init: At 3FCE0000 len 0000EE34 (59 KiB): STACK/DRAM
I (1556) heap_init: At 600FE000 len 00002000 (8 KiB): RTCRAM
I (1556) spiram: Adding pool of 8192K of external SPI memory to heap allocator
I (1557) spi_flash: detected chip: gd
I (1557) spi_flash: flash io: qio
I (1558) sleep: Configure to isolate all GPIO pins in sleep state
I (1558) sleep: Enable automatic switching of GPIO sleep configuration
I (1558) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.
I (1559) spiram: Reserving pool of 32K of internal memory for DMA/internal allocations
I (1559) gpio: GPIO[1]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:3 
I (1629) bsp_spiffs: Partition size: total: 956561, used: 170429
I (1629) gpio: GPIO[4]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (1629) gpio: GPIO[48]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (1749) gpio: GPIO[45]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (1749) bsp_tp: Detected touch panel at 0x24. Vendor : Parade Tech
I (1789) gpio: GPIO[3]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0 
```


