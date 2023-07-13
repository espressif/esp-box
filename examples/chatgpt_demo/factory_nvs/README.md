## TinyUF2 NVS: Adding USB capabilities to ESP-Box

This standalone project provides support for dumping NVS key-value pairs, allowing users to access and modify the NVS data. They can retrieve the NVS data, make necessary changes, and write the updated values back to the NVS using.

## Build

Run `idf.py build` to build and flash the project.

## Flash 

Since this project functions as a "factory app" for chatgpt_demo, we exclusively employ the "esptool" to flash the binaries. Checkout the [partition table](../partitions.csv)

```bash
python -m esptool -p /dev/ttyACM0 --chip esp32s3 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x700000 build/factory_nvs.bin

```

Check the [original implementation of Tinyuf2](https://github.com/espressif/esp-iot-solution/tree/master/components/usb/esp_tinyuf2) for more information. For more details of follow the [chatgpt_demo](../README.md) project. 