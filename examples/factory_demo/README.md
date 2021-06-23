# ESP32-S3-Cube Factory Demo

This is the factory demo for ESP32-S3-Cube development board.

## How to use example

Follow detailed instructions provided specifically for this example. 

1. Connect the development board to PC with USB-C cable. If there is no issue with LCD, display will on and show something.
2. Probe procedure will run after LCD on. Such ICs will be detected if there is no problem with them:
   - IMU - MPU6886, Device Addr : 0x68.
   - Audio Decoder : ES8311, output only mode. Device addr : 0x18.
   - Audio Coder : ES7210 is configured as output only mode. Device addr : 0x40
   - Touch panel : FT5436 or other touch IC. Device addr : 0x38.
   - Crypto Authentication IC : ATECC608. Device addr : 0x35.
3. Press "OK" button to start tests.

## Troubleshooting

* Program upload failure

    * Hardware connection is not correct: run `idf.py -p PORT monitor`, and reboot your board to see if there are any output logs.
    * The baud rate for downloading is too high: lower your baud rate in the `menuconfig` menu, and try again.

## Technical support and feedback

Please use the following feedback channels:

* For technical queries, go to the [esp32.com](https://esp32.com/) forum
* For a feature request or bug report, create a [GitHub issue](https://github.com/espressif/esp-idf/issues)

We will get back to you as soon as possible.
