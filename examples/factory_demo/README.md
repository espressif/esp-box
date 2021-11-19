# ESP32-S3-Box Factory Demo

This is the factory demo for ESP32-S3-Box development board.

## How to Compile

Since some bug fixes of esp-idf may not be synced to GitHub, you need to manually apply some patches to build the example.

### Apply Patch

For Linux and macOS, you can apply the patch by entering the following command on the command line / terminal:

```shell
cd /path/to/esp-box/idf_patch
python3 apply_patch.py -d /path/to/esp-idf
. /path/to/esp-idf/export.sh
```

For Windows users, you may need to do the following manually:

- Change the working directory to the folder where esp-idf is located
- Run the following commands in cmd in sequence：
  - `git fetch origin`
  - `git checkout 35b20cadce65ce79c14cf2018efc87c44d71ab21`
  - `git apply X:\path\to/esp-box\idf_patch\idf_patch.patch`
- Copy the files in the `idf_patch\components\esp_phy` folder and replace them to the same location under esp-idf in turn
- Run `install.bat` in the esp-idf directory
- Run `export.bat` in the esp-idf directory
- Enter the esp-box repo to compile the project you want

### Compile Example

The project provides `sdkconfig.defaults.cn` and `sdkconfig.defaults.en`, which are the default configuration files corresponding to Chinese and English respectively. By replacing `sdkconfig.defaults` with the above file, deleting `sdkconfig`, rebuilding and burning, you can burn routines in the specified language to ESP-Box.

## How to Use

### Hardware Required

* A ESP32-S3-Box
* An USB-C cable for power supply and programming
* An RGB LED used to transmit encoded IR signals 

example connection:

|         | ESP32-S3-Box |
|  :---:  | :-----------: |
|   RED   |     IO 39     |
|  GREEN  |     IO 40     |
|  BLUE   |     IO 41     |

### Wake Up Device

Say "Hi esp" to wake up the device. Then say command word list is as follows after animate shows on screen:

- Turn On/Off The Light
- Turn Red / Green / Blue / White
- Custom Color

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

