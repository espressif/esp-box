* [中文版本](./firmeware_update_cn.md)

## Update ESP32-S3-BOX Firmware

ESP32-S3-BOX supports updating firmware through USB, you just need:

1. USB Type-C cable
2. [Latest firmware bin file](https://github.com/espressif/esp-box/releases)

### Download Latest Firmware

For latest firmware, please refer to: https://github.com/espressif/esp-box/releases

1. Choose latest release version
2. Click `Assets` to show more files
3. Choose a `xxxxxx.bin` to download

<div align="center">
<img src="_static/bin_download.png">
</div>

### Firmware update for Windows 

We recommend using `Windows 10` and above system. Under `Windows 10` system, the driver of `USB-Serial-Jtag` will be downloaded automatically. if you use the `Windows 7`, please download and install [USB-Serial-JTAG drive](https://dl.espressif.com/dl/idf-driver/idf-driver-esp32-usb-jtag-2021-07-15.zip) manully.

1. [Download latest firmware](https://github.com/espressif/esp-box/releases)
2. Connect ESP32-S3-BOX to the computer through USB Type-C cable.

   <div align="center">
   <img src="_static/plug_power.png">
   </div>

3. Please make sure the computer is connected to the Internet firstly. When the driver is installed, you can find the devices in `Device Manager`. There will be two more new devices  `COMX` (`COM2` for example) and `USB JTAG/serial debug unit`, the former is used to download firmware or output program logs, the latter is used for JTAG debugging.

   <div align="center">
   <img src="_static/device_manager_usb_serial_jtag.png">
   </div>

4. Download [Windows download tool](https://www.espressif.com/sites/default/files/tools/flash_download_tool_3.9.2_0.zip) and unzip it to any folder, then run the executable file `flash_download_tool_x.x.x.exe`
5. Please choose `chipType`: `ESP32S3`, `workMode`: `develop`, `loadMode`: `usb`, then click `OK` to enter the download ui:

   <div align="center">
   <img src="_static/dl_tool_windows.png">
   </div>

6. Configure the download tool follow the instruction below:
   1. Choose the path of firmwire `xxxx.bin` , and set the address to `0x0`
   2. Select the COM port, like `COM2` for this PC.
   3. Click `START` to start download.

   <div align="center">
   <img src="_static/dl_tool_windows_2.png">
   </div>

7. After downloading, `FINISH` will shown in the tool, then **please reboot to run the new firmware!**

   <div align="center">
   <img src="_static/dl_tool_windows_3.png">
   </div>


### Firmware update for Linux 

1. [Download latest firmware](https://github.com/espressif/esp-box/releases)
2. Connect ESP32-S3-BOX to the computer through USB Type-C cable，there is no need to install the driver of `USB-Serial-Jtag` under Linux system.
3. Install `esptool`, input following commands in `Terminal` (`pip` can be specified as `pip3`) :

    ```
    pip install esptool
    ```

   <div align="center">
   <img src="_static/linux_install_esptool.png">
   </div>

4. Download firmware using following instruction (`pip` can be specified as `pip3`)：

    ```
    python -m esptool --chip esp32s3 write_flash 0x0 download_path/test_bin.bin
    ```

   1. `0x0` is the flash address 
   2. `download_path/test_bin.bin` is the name and path the downloaded firmware.

5. After updating, download tool will prompt `Hash of data verified`, then **please reboot to run the new firmware!**

   <div align="center">
   <img src="_static/linux_download.png">
   </div>

### Firmware update for macOS

1. [Download latest firmware](https://github.com/espressif/esp-box/releases)

2. Connect ESP32-S3-BOX to the computer through USB Type-C cable，there is no need to install the driver of `USB-Serial-Jtag` under macOS.

3. Install `esptool`, input following codes in `Terminal` (`pip3` can be specified as `pip`) :

    ```
    pip3 install esptool
    ```

    <div align="center">
    <img src="_static/macos_install_esptool.png">
    </div>

4. Download firmware using following commands (`pip3` can be specified as `pip`):

   ```
   python3 -m esptool --chip esp32s3 write_flash 0x0 download_path/test_bin.bin
   ```

   1. `0x0` is the flash address 
   2. `download_path/test_bin.bin` is the name and path for the downloaded firmware.

5. After updating, download tool will prompt `Hash of data verified`, then **please reboot to run the new firmware!**

   <div align="center">
   <img src="_static/macos_download.png">
   </div>