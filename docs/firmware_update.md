* [中文版本](./firmware_update_cn.md)

# Update Firmware

The BOX series development boards support updating firmware through USB, you just need:

1. USB Type-C cable
2. Latest firmware bin file

**Important Note:**
> If USB download fails, please hold down `Boot` button and then press `Reset` button to initiates Firmware Download mode.

## Latest Firmware Download

For the latest firmware, please refer to: https://github.com/espressif/esp-box/releases

1. Choose the latest release version
2. Click `Assets` to show more files
3. Choose an `xxxxxx.bin` to download

<div align="center">
<img src="_static/bin_download.png">
</div>

## Firmware update

Please follow the link below correspond to your OS to update the firmware.

| [Windows](#firmware-update-for-windows) | [Linux](#firmware-update-for-linux) | [macOS](#firmware-update-for-macos) |
|:----:|:----:|:----:|
| [<img src="_static/windows-logo.png" width="320" width="240" align="center" />](#firmware-update-for-windows) | [<img src="_static/linux-logo.png" width="320" width="240" align="center" />](#firmware-update-for-linux) | [<img src="_static/macos-logo.jpg" width="320" width="240" align="center" />](#firmware-update-for-macos) |

#### Firmware update for Windows 

We recommend using `Windows 10` and above system. Under `Windows 10` system, the driver of `USB-Serial-Jtag` will be downloaded automatically. If you use the `Windows 7`, please download and install [USB-Serial-JTAG drive](https://dl.espressif.com/dl/idf-driver/idf-driver-esp32-usb-jtag-2021-07-15.zip) manually.

1. [Download latest firmware](#latest-firmware-download)
2. Connect your development board to the computer through USB Type-C cable.
3. Please make sure the computer is connected to the Internet first. When the driver is installed, you can find two new devices appear on `Device Manager` list, `COMX` (`COM2` for example) and `USB JTAG/serial debug unit`, the former is used to download firmware or output program logs, the latter is used for JTAG debugging.

   <div align="center">
   <img src="_static/device_manager_usb_serial_jtag.png">
   </div>

4. Download [Windows download tool](https://www.espressif.com/sites/default/files/tools/flash_download_tool_3.9.2_0.zip) and unzip it to any folder, then run the executable file `flash_download_tool_x.x.x.exe`
5. Please choose `chipType`: `ESP32S3`, `workMode`: `develop`, `loadMode`: `usb`, then click `OK` to enter the download tool config interface.

   <div align="center">
   <img src="_static/dl_tool_windows.png">
   </div>

6. Follow the instruction below to configure the downloaded tool:
   1. Choose the path of firmware `xxxx.bin`, then set the download address to `0x0`.
   2. Select the COM port, like `COM2` for this PC.
   3. Click `START` to start the downloading.

   <div align="center">
   <img src="_static/dl_tool_windows_2.png">
   </div>

7. After downloading, `FINISH` will appear on the tool. Next, **please reboot to run the new firmware!**

   <div align="center">
   <img src="_static/dl_tool_windows_3.png">
   </div>


#### Firmware update for Linux 

1. [Download latest firmware](#latest-firmware-download)
2. Connect your development board to the computer through USB Type-C cable, there is no need to install the driver of `USB-Serial-Jtag` under Linux system.
3. Install `esptool`, input the following commands in `Terminal` (`pip` can be specified as `pip3`) :

    ```
    pip install esptool
    ```

   <div align="center">
   <img src="_static/linux_install_esptool.png">
   </div>

4. Follow the instruction to download the firmware (`pip` can be specified as `pip3`)：

    ```
    python -m esptool --chip esp32s3 write_flash 0x0 download_path/test_bin.bin
    ```

   1. `0x0` is the fixed flash address 
   2. `download_path/test_bin.bin` need be replaced with your firmware path and name.

5. After updating, the download tool will prompt `Hash of data verified`. Next, **please reboot to run the new firmware!**

   <div align="center">
   <img src="_static/linux_download.png">
   </div>

#### Firmware update for macOS

1. [Download latest firmware](#latest-firmware-download)

2. Connect your development board to the computer through USB Type-C cable, there is no need to install the driver of `USB-Serial-Jtag` under macOS.

3. Install `esptool`, input the following codes in `Terminal` (`pip3` can be specified as `pip`) :

    ```
    pip3 install esptool
    ```

    <div align="center">
    <img src="_static/macos_install_esptool.png">
    </div>

4. Follow the instruction to download the firmware (`pip3` can be specified as `pip`):

   ```
   python3 -m esptool --chip esp32s3 write_flash 0x0 download_path/test_bin.bin
   ```

   1. `0x0` is the fixed flash address.
   2. `download_path/test_bin.bin` need be replaced with your firmware path and name.

5. After updating, the download tool will prompt `Hash of data verified`. Next, **please reboot to run the new firmware!**

   <div align="center">
   <img src="_static/macos_download.png">
   </div>