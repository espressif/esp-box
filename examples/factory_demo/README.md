# Factory Example

| Board             | Support Status |
| ----------------- | -------------- |
| ESP32-S3-BOX      | YES            |
| ESP32-S3-BOX-Lite | YES            |

The factory_demo is a built-in firmware of ESP-BOX series products, which integrates LVGL, ESP-Rainmaker and ESP-SR.

## How to use example

Please check the [User-Guide](../../docs/getting_started.md).

### Hardware Required

* A ESP32-S3-Box or ESP32-S3-BOX-Lite
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
load:0x3fcd0108,len:0x17a4
load:0x403b6000,len:0xe0c
load:0x403ba000,len:0x3068
entry 0x403b626c
I (24) boot: ESP-IDF v4.4-129-g6934a0164a-dirty 2nd stage bootloader
I (25) boot: compile time 19:04:00
I (25) boot: chip revision: 0
I (28) qio_mode: Enabling default flash chip QIO
I (33) boot.esp32s3: Boot SPI Speed : 80MHz
I (38) boot.esp32s3: SPI Mode       : QIO
I (43) boot.esp32s3: SPI Flash Size : 16MB
I (48) boot: Enabling RNG early entropy source...
I (53) boot: Partition Table:
I (57) boot: ## Label            Usage          Type ST Offset   Length
I (64) boot:  0 sec_cert         Unknown data     01 06 0000d000 00003000
I (71) boot:  1 nvs              WiFi data        01 02 00010000 00006000
I (79) boot:  2 otadata          OTA data         01 00 00016000 00002000
I (86) boot:  3 phy_init         RF data          01 01 00018000 00001000
I (94) boot:  4 fctry            WiFi data        01 02 00019000 00006000
I (101) boot:  5 ota_0            OTA app          00 10 00020000 0039d000
I (109) boot:  6 storage          Unknown data     01 82 003bd000 0028a000
I (116) boot:  7 model            Unknown data     01 82 00647000 007e9000
I (124) boot: End of partition table
I (128) esp_image: segment 0: paddr=00020020 vaddr=3c140020 size=1fc3d4h (2081748) map
I (452) esp_image: segment 1: paddr=0021c3fc vaddr=3fca5500 size=03c1ch ( 15388) load
I (455) esp_image: segment 2: paddr=00220020 vaddr=42000020 size=13d828h (1300520) map
I (655) esp_image: segment 3: paddr=0035d850 vaddr=3fca911c size=0163ch (  5692) load
I (657) esp_image: segment 4: paddr=0035ee94 vaddr=40378000 size=1d4f8h (120056) load
I (684) esp_image: segment 5: paddr=0037c394 vaddr=50000000 size=00014h (    20) load
I (696) boot: Loaded app from partition at offset 0x20000
I (696) boot: Disabling RNG early entropy source...
I (696) opi psram: vendor id : 0x0d (AP)
I (701) opi psram: dev id    : 0x02 (generation 3)
I (706) opi psram: density   : 0x03 (64 Mbit)
I (711) opi psram: good-die  : 0x01 (Pass)
I (716) opi psram: Latency   : 0x01 (Fixed)
I (721) opi psram: VCC       : 0x01 (3V)
I (725) opi psram: SRF       : 0x01 (Fast Refresh)
I (731) opi psram: BurstType : 0x01 (Hybrid Wrap)
I (736) opi psram: BurstLen  : 0x01 (32 Byte)
I (741) opi psram: Readlatency  : 0x02 (10 cycles@Fixed)
I (747) opi psram: DriveStrength: 0x00 (1/1)
W (752) PSRAM: DO NOT USE FOR MASS PRODUCTION! Timing parameters will be updated in future IDF version.
I (763) spiram: Found 64MBit SPI RAM device
I (767) spiram: SPI RAM mode: sram 80m
I (771) spiram: PSRAM initialized, cache is in normal (1-core) mode.
I (778) cpu_start: Pro cpu up.
I (782) cpu_start: Starting app cpu, entry point is 0x403794d8
0x403794d8: call_start_cpu1 at /home/zhouli/esp/esp-idf/master/components/esp_system/port/cpu_start.c:160

I (0) cpu_start: App cpu up.
I (1077) spiram: SPI SRAM memory test OK
I (1086) cpu_start: Pro cpu start user code
I (1086) cpu_start: cpu freq: 240000000
I (1086) cpu_start: Application information:
I (1087) cpu_start: Project name:     lite_demo
I (1087) cpu_start: App version:      6d88257-dirty
I (1087) cpu_start: Compile time:     Mar  2 2022 17:44:23
I (1087) cpu_start: ELF file SHA256:  500f0cf0f4dbfedd...
I (1087) cpu_start: ESP-IDF:          v4.4-129-g6934a0164a-dirty
I (1088) heap_init: Initializing. RAM available for dynamic allocation:
I (1088) heap_init: At 3FCB1690 len 0002E970 (186 KiB): D/IRAM
I (1088) heap_init: At 3FCE0000 len 0000EE34 (59 KiB): STACK/DRAM
I (1089) heap_init: At 600FE000 len 00002000 (8 KiB): RTCRAM
I (1089) spiram: Adding pool of 8192K of external SPI memory to heap allocator
I (1089) spi_flash: detected chip: gd
I (1090) spi_flash: flash io: qio
I (1090) sleep: Configure to isolate all GPIO pins in sleep state
I (1091) sleep: Enable automatic switching of GPIO sleep configuration
I (1091) coexist: coexist rom version e7ae62f
I (1091) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.
I (1092) spiram: Reserving pool of 8K of internal memory for DMA/internal allocations
I (1119) main: System Info Trace
        Description     Internal        SPIRAM
Current Free Memory     220191          8386191
Largest Free Block      151552          8257536
Min. Ever Free Size     219423          8386191
I (1119) codec: Detected codec at 0x08. Name : ES7243
I (1119) codec: Detected codec at 0x10. Name : ES8156
I (1121) bsp boards: Detected board: [S3_BOX_LITE]
I (1121) gpio: GPIO[0]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0 
I (1121) adc button: Characterized using Default Vref
I (1122) I2S: DMA Malloc info, datalen=blocksize=640, dma_buf_count=6
I (1123) I2S: DMA Malloc info, datalen=blocksize=640, dma_buf_count=6
I (1123) I2S: I2S0, MCLK output by GPIO2
I (1124) codec: Detected codec at 0x08. Name : ES7243
I (1124) codec: Detected codec at 0x10. Name : ES8156
I (1130) codec: init ES7243
I (1131) codec: init ES8156
I (1131) gpio: GPIO[46]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (1132) gpio: GPIO[4]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (1132) gpio: GPIO[48]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (1252) gpio: GPIO[45]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (1252) lv_port: Try allocate two 320 * 20 display buffer, size:25600 Byte
I (1254) lv_port: Add KAYPAD input device to LVGL
I (1876) bsp_spiffs: Partition size: total: 7616846, used: 7341248
I (2040) bsp_spiffs: Partition size: total: 2439971, used: 1367197
I (2057) ui_main: Input device type is keypad
Traverse directory /spiffs/mp3
I (2095) SR_AFE: Initial auido front-end, total channel: 3, mic num: 2, ref num: 1

I (2097) SR_AFE: aec_init: 0, se_init: 1, vad_init: 1

I (2098) SR_AFE: wakenet_init: 1, voice_communication_init: 0
```
