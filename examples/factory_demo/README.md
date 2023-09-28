# Factory Example

| Board             | Support Status |
| ----------------- | -------------- |
| ESP32-S3-BOX      | YES            |
| ESP32-S3-BOX-Lite | YES            |
| ESP32-S3-BOX-3    | YES            |


The factory_demo is a built-in firmware of ESP32-S3-BOX-3, which integrates LVGL, ESP-Rainmaker and ESP-SR. This example is also adaptable for use with ESP32-S3-BOX and ESP32-S3-BOX-Lite. You can choose your development board via the menuconfig settings when compiling the example.

## How to use example

<font color="red">[Note]: </font>This example requires ESP-IDF release/v5.1 or later.

Please check the [User Guide](../../docs/getting_started.md) for more details about how to use the demo.

### Hardware Required

* A ESP32-S3-BOX-3
* A USB-C cable for power supply and programming

### Build and Flash

Run `idf.py flash monitor` to build, flash and monitor the project.

Once a complete flash process has been performed, you can use `idf.py app-flash monitor` to reduce the flash time.

(To exit the serial monitor, type `Ctrl-]`. Please reset the development board f you cannot exit the monitor.)