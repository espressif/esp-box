## USB Stream LCD Display Example

| Board             | Support Status |
| ----------------- | -------------- |
| ESP32-S3-BOX      | NO             |
| ESP32-S3-BOX-Lite | NO             |
| ESP32-S3-BOX-3    | YES            |

This routine demonstrates how to use the [usb_stream](https://components.espressif.com/components/espressif/usb_stream) component to acquire a USB camera image and display it adaptively on the LCD screen.

* Pressing the boot button can switch the display resolution. 
* For better performance, please use ESP-IDF release/v5.0 or above versions.
* Currently only images with a resolution smaller than the screen resolution are displayed
* When the image width is equal to the screen width, the refresh rate is at its highest.

## Hardware

* An ESP32-S3-BOX-3 development board with 320*240 LCD.
* A [USB camera](https://docs.espressif.com/projects/espressif-esp-iot-solution/zh_CN/latest/usb/usb_stream.html#id1) (Can be connected to USB port or Expansion Connector).
* An USB Type-C cable for Power supply and programming (Please connect to UART port instead of USB port)

Note:
  Error message with `A fatal error occurred: Could not open /dev/ttyACM0, the port doesn't exist`: Please first make sure development board connected, then make board into "Download Boot" mode to upload by following steps:
  1. keep pressing  "BOOT(SW2)" button
  2. short press "RST(SW1)" button
  3. release "BOOT(SW2)".
  4. upload program and reset

## PSRAM 120M DDR

The PSRAM 120M DDR feature is intended to achieve the best performance of RGB LCD. It is only available with ESP-IDF **release/v5.1** and above. It can be used by enabling the `IDF_EXPERIMENTAL_FEATURES`, `SPIRAM_SPEED_120M`, `SPIRAM_MODE_OCT` options. see [here](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/flash_psram_config.html#all-supported-modes-and-speeds) for more details.

**Note: The PSRAM 120 MHz DDR is an experimental feature and it has temperature risks as below.**
  * Cannot guarantee normal functioning with a temperature higher than 65 degrees Celsius.
  * Temperature changes can also cause the crash of accessing to PSRAM/Flash, see [here](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/flash_psram_config.html#all-supported-modes-and-speeds) for more details.

### How To Use 

Note:       

1. First delete existing `build`, `sdkconfig` , `sdkconfig.old`
```
rm -rf build sdkconfig sdkconfig.old
```

2. Use the command line to enable the relevant configuration
```
idf.py -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.psram_octal_120m" reconfigure
```

3. Compile and burn
```
idf.py build flash monitor
```