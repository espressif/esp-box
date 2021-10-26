# ESP32-S3-Cube Factory Demo

This is the factory demo for ESP32-S3-Cube development board.

## How to Use

### Hardware Required

* A ESP32-S3-Cube
* An USB-C cable for power supply and programming
* An RGB LED used to transmit encoded IR signals 

example connection:

|         | ESP32-S3-Cube |
|  :---:  | :-----------: |
|   RED   |     IO 39     |
|  GREEN  |     IO 40     |
|  BLUE   |     IO 41     |

### Wake Up Device

Say "Hi esp" to wake up the device. Then say command word list is as follows after animate shows on screen:

- Turn On/Off The Light
- Turn Red / Green / Blue / White

The detected command word will be displayed on the screen. If the LED is correctly connected to the device, its color will change according to the command word.

### Modify the command word

- Click the Wi-Fi button at the top.
- Scan the QR code to connect to the AP of the device.
- Scan the QR code again to enter the configuration page.
- Click on the device and enter the control page.
- Modify "Voice command" and click "Save".
- Try your new command word to turn on / off the light

### Build and Flash

Run `idf.py flash monitor` to build, flash and monitor the project.

Once a complete flash process has been performed, you can use `idf.py app-flash monitor` to reduce the flash time.

(To exit the serial monitor, type `Ctrl-]`. Please reset the development board f you cannot exit the monitor.)

## Example Output

Run this example, you will see the following output log:

```
SPIWP:0xee
mode:DIO, clock div:1
load:0x3fcd0108,len:0x51c
load:0x403b6000,len:0x93c
load:0x403ba000,len:0x2c48
entry 0x403b6164
I (33) opi psram: vendor id : 0x0d (AP)
I (34) opi psram: dev id    : 0x02 (generation 3)
I (34) opi psram: density   : 0x03 (64 Mbit)
I (37) opi psram: good-die  : 0x01 (Pass)
I (42) opi psram: Latency   : 0x01 (Fixed)
I (46) opi psram: VCC       : 0x01 (3V)
I (51) opi psram: SRF       : 0x01 (Fast Refresh)
I (56) opi psram: BurstType : 0x01 (Hybrid Wrap)
I (62) opi psram: BurstLen  : 0x01 (32 Byte)
I (67) opi psram: Readlatency  : 0x02 (10 cycles@Fixed)
I (73) opi psram: DriveStrength: 0x00 (1/1)
W (77) PSRAM: DO NOT USE FOR MASS PRODUCTION! Timing parameters will be updated in future IDF version.
I (88) spiram: Found 64MBit SPI RAM device
I (92) spiram: SPI RAM mode: sram 80m
I (96) spiram: PSRAM initialized, cache is in normal (1-core) mode.
I (103) cpu_start: Pro cpu up.
I (107) cpu_start: Starting app cpu, entry point is 0x403796c4
0x403796c4: call_start_cpu1 at /home/zhe/esp/gitlab/idf/esp-idf-hmi/components/esp_system/port/cpu_start.c:156

I (0) cpu_start: App cpu up.
I (402) spiram: SPI SRAM memory test OK
I (411) cpu_start: Pro cpu start user code
I (411) cpu_start: cpu freq: 240000000
I (411) cpu_start: Application information:
I (411) cpu_start: Project name:     factory_demo
I (411) cpu_start: App version:      b720353-dirty
I (411) cpu_start: Compile time:     Nov  9 2021 11:25:23
I (412) cpu_start: ELF file SHA256:  e12c5b6471e7e231...
I (412) cpu_start: ESP-IDF:          v5.0-dev-19-g290c805aa8-dirty
I (412) heap_init: Initializing. RAM available for dynamic allocation:
I (413) heap_init: At 3FCB4708 len 0002B8F8 (174 KiB): D/IRAM
I (413) heap_init: At 3FCE0000 len 0000EE34 (59 KiB): STACK/DRAM
I (413) spiram: Adding pool of 8192K of external SPI memory to heap allocator
I (414) spi_flash: detected chip: gd
I (414) spi_flash: flash io: qio
I (415) sleep: Configure to isolate all GPIO pins in sleep state
I (415) sleep: Enable automatic switching of GPIO sleep configuration
I (415) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.
```

## Troubleshooting

* Program upload failure

    * Hardware connection is not correct: run `idf.py -p PORT monitor`, and reboot your board to see if there are any output logs.
    * Try to put ESP32-S3 into download mode. To do this, keep GPIO0 low while toggling reset. On many development boards, the “Boot” button is connected to GPIO0, and you can press “Reset” button while holding “Boot”.
* LCD does not display properly or flickers
    * Make sure that the MUTE button is not pressed, and the yellow indicator light on the top of the development board is not lit.
* No response after scanning the QR code
    * Try to manually connect to the AP displayed on the screen and enter the URL in the browser

## Technical support and feedback

Please use the following feedback channels:

* For technical queries, go to the [esp32.com](https://esp32.com/) forum
* For a feature request or bug report, create a [GitHub issue](https://github.com/espressif/esp-box/issues)

We will get back to you as soon as possible.

