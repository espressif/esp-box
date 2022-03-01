# MP3 Player Example

| Board             | Support Status |
| ----------------- | -------------- |
| ESP32-S3-BOX      | YES            |
| ESP32-S3-BOX-Lite | NO             |

Play MP3 music on ESP-BOX.

This demo will scan the files in the specified directory (`/spiffs` by default) and try to decode and play. You can manually mount the SD card and switch to play MP3 files from the SD card.

We will support switching the sample rate and the number of channels in the next version to make it more flexible.

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
load:0x3fcd0108,len:0x4ac
load:0x403b6000,len:0x93c
load:0x403ba000,len:0x2c28
entry 0x403b6164
I (845) opi psram: vendor id : 0x0d (AP)
I (845) opi psram: dev id    : 0x02 (generation 3)
I (845) opi psram: density   : 0x03 (64 Mbit)
I (849) opi psram: good-die  : 0x01 (Pass)
I (853) opi psram: Latency   : 0x01 (Fixed)
I (858) opi psram: VCC       : 0x01 (3V)
I (863) opi psram: SRF       : 0x01 (Fast Refresh)
I (868) opi psram: BurstType : 0x01 (Hybrid Wrap)
I (874) opi psram: BurstLen  : 0x01 (32 Byte)
I (879) opi psram: Readlatency  : 0x02 (10 cycles@Fixed)
I (885) opi psram: DriveStrength: 0x00 (1/1)
W (890) PSRAM: DO NOT USE FOR MASS PRODUCTION! Timing parameters will be updated in future IDF version.
I (900) spiram: Found 64MBit SPI RAM device
I (904) spiram: SPI RAM mode: sram 80m
I (909) spiram: PSRAM initialized, cache is in normal (1-core) mode.
I (916) cpu_start: Pro cpu up.
I (920) cpu_start: Starting app cpu, entry point is 0x403793e8
0x403793e8: call_start_cpu1 at /home/zhe/esp/esp-idf/components/esp_system/port/cpu_start.c:156

I (0) cpu_start: App cpu up.
I (1215) spiram: SPI SRAM memory test OK
I (1224) cpu_start: Pro cpu start user code
I (1224) cpu_start: cpu freq: 240000000
I (1225) cpu_start: Application information:
I (1225) cpu_start: Project name:     mp3_demo
I (1225) cpu_start: App version:      41aee698-dirty
I (1225) cpu_start: Compile time:     Dec  6 2021 15:17:24
I (1225) cpu_start: ELF file SHA256:  608fee9998f93684...
I (1226) cpu_start: ESP-IDF:          v4.4-dev-3675-g35b20cadce-dirty
I (1226) heap_init: Initializing. RAM available for dynamic allocation:
I (1226) heap_init: At 3FCA2CC8 len 0003D338 (244 KiB): D/IRAM
I (1226) heap_init: At 3FCE0000 len 0000EE34 (59 KiB): STACK/DRAM
I (1227) heap_init: At 600FE000 len 00001FF0 (7 KiB): RTCRAM
I (1227) spiram: Adding pool of 8192K of external SPI memory to heap allocator
I (1228) spi_flash: detected chip: gd
I (1228) spi_flash: flash io: qio
I (1228) sleep: Configure to isolate all GPIO pins in sleep state
I (1229) sleep: Enable automatic switching of GPIO sleep configuration
I (1229) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.
I (1230) spiram: Reserving pool of 8K of internal memory for DMA/internal allocations
I (1230) gpio: GPIO[1]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:3 
I (1230) gpio: GPIO[46]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (2100) bsp_spiffs: Partition size: total: 13486481, used: 12997282
I (2100) gpio: GPIO[4]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (2100) gpio: GPIO[48]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (2220) gpio: GPIO[45]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (2220) bsp_tp: Detected touch panel at 0x24. Vendor : Parade Tech
I (2260) gpio: GPIO[3]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0 
I (2690) audio: File : Canon.mp3
I (2840) audio: File : For Elise.mp3
I (2910) audio: File : Waka Waka.mp3
E (2910) audio: audio_get_name_from_index(125): File index out of range
I (2910) codec: Detected codec at 0x40. Name : ES7210
I (2910) codec: Detected codec at 0x18. Name : ES8311
I (2920) ES7210: ES7210 in Slave mode
I (2930) ES7210: Enable ES7210_INPUT_MIC1
I (2930) ES7210: Enable ES7210_INPUT_MIC2
I (2930) ES7210: Enable ES7210_INPUT_MIC3
I (2930) ES7210: Enable ES7210_INPUT_MIC4
I (2940) ES7210: The ES7210_CLOCK_OFF_REG01 value before stop is 0
I (2940) ES7210: Enable ES7210_INPUT_MIC1
I (2940) ES7210: Enable ES7210_INPUT_MIC2
I (2940) ES7210: Enable ES7210_INPUT_MIC3
I (2940) ES7210: Enable ES7210_INPUT_MIC4
I (2940) DRV8311: ES8311 in Slave mode
I (2960) I2S: DMA Malloc info, datalen=blocksize=640, dma_buf_count=6
I (2970) I2S: DMA Malloc info, datalen=blocksize=640, dma_buf_count=6
I (2970) I2S: I2S0, MCLK output by GPIO2
I (2980) audio: start to decode /spiffs/Canon.mp3
```
