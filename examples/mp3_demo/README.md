# MP3 Player Example

| Board             | Support Status |
| ----------------- | -------------- |
| ESP32-S3-BOX      | YES            |
| ESP32-S3-BOX-Lite | YES            |
| ESP32-S3-BOX-3    | YES            |

Play MP3 music on ESP-BOX.

This demo will scan the files in the specified directory (`/spiffs` by default) and try to decode and play. You can manually mount the SD card and switch to play MP3 files from the SD card.

We will support switching the sample rate and the number of channels in the next version to make it more flexible.

## How to use example

### Hardware Required

* A ESP32-S3-Box or ESP32-S3-BOX-Lite
* A USB-C cable for power supply and programming

Follow detailed instructions provided specifically for this example. 

### Build and Flash

Run `idf.py flash monitor` to build, flash and monitor the project.

Once a complete flash process has been performed, you can use `idf.py app-flash monitor` to reduce the flash time.

(To exit the serial monitor, type `Ctrl-]`. Please reset the development board f you cannot exit the monitor.)

