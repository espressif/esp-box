* [English Version](./firmware_update.md)

# 更新固件

BOX 系列开发板支持使用 USB 接口更新固件，您仅需要准备：

1. 一根 USB Type-C 数据线
2. 最新版本固件（bin 文件）

**需要提醒：**
> 如遇 USB 下载失败，请按住 `Boot` 按键的同时按一下 `Reset` 按键进入“固件下载”模式。

## 下载最新版本固件

请访问 https://github.com/espressif/esp-box/releases 下载最新发布的固件。

1. 请先选择一个固件（建议使用最新发布的固件）。
2. 点击 `Assets` 展开下载链接。
3. 选择一个 `xxxxxx.bin` 进行下载。

<div align="center">
<img src="_static/bin_download.png" width="70%">
</div>

## 固件更新

不同操作系统的固件更新方法，请参考以下链接：

| [<center> Windows </center>](#windows-系统更新固件说明) | [<center> Linux </center>](#linux-系统更新固件说明) | [<center> macOS </center>](#macos-系统更新固件说明) |
|:----:|:----:|:----:|
| [<img src="_static/windows-logo.png" width="40" align="center" />](#windows-系统更新固件说明) | [<img src="_static/linux-logo.png" width="40" align="center" />](#linux-系统更新固件说明) | [<img src="_static/macos-logo.jpg" width="40" align="center" />](#macos-系统更新固件说明) |

### Windows 系统更新固件说明

我们推荐使用 `Windows 10` 及以上版本，在该系统下 `USB-Serial-Jtag` 的驱动将联网自动下载。如果使用 `Windows 7` 系统，请手动下载 [USB-Serial-JTAG 驱动](https://dl.espressif.com/dl/idf-driver/idf-driver-esp32-usb-jtag-2021-07-15.zip) 并安装。

1. [下载最新版本固件](#下载最新版本固件)。
2. 使用 USB Type-C 数据线将开发板接入电脑。
3. 初次使用，请确保电脑已联网。驱动正常自动安装后，我们能在设备管理器看到以下设备。这里将多出两个新的设备 `COMX`（此电脑为 `COM2`）和 `USB JTAG/serial debug unit`，前者用于下载固件和输出程序日志，后者用于 JTAG 调试。

   <div align="center">
   <img src="_static/device_manager_usb_serial_jtag_cn.png" width="450">
   </div>

4. 下载 [Windows download tool](https://www.espressif.com/sites/default/files/tools/flash_download_tool_3.9.2_0.zip)，并解压到任意文件夹，然后请双击打开下载工具可执行文件 `flash_download_tool_x.x.x.exe`。
5. 请选择 `chipType` 为 `ESP32S3`, `workMode` 为 `develop`, `loadMode` 为 `usb`，之后点击 `OK` 进入下载工具界面：

   <div align="center">
   <img src="_static/dl_tool_windows.png" width="180">
   </div>

6. 请按照下图指示配置下载工具：
   1. 首先选择 `xxxx.bin` 路径，将地址设置为 `0x0`。
   2. 选择下载端口 COMX，此电脑为 `COM2`。 
   3. 点击 `START` 开始固件下载。

   <div align="center">
   <img src="_static/dl_tool_windows_2.png" width="300">
   </div>

7. 下载完成后，工具将提示 `FINISH`，之后**按下 `Reset` 按键，即可进入新程序！**

   <div align="center">
   <img src="_static/dl_tool_windows_3.png" width="300">
   </div>


### Linux 系统更新固件说明

1. [下载最新版本固件](#下载最新版本固件)。
2. 使用 USB Type-C 数据线将开发板接入电脑，`USB-Serial-Jtag` 在 Linux 系统下无需安装驱动。
3. 安装下载工具 `esptool`，请打开 `终端` ，并输入以下指令（`pip` 也可指定为 `pip3`）：

    ```
    pip install esptool
    ```

   <div align="center">
   <img src="_static/linux_install_esptool.png" width="480">
   </div>

4. 请使用以下指令下载固件（`python` 也可指定为 `python3`）：

    ```
    python -m esptool --chip esp32s3 write_flash 0x0 download_path/test_bin.bin
    ```

   1. `0x0` 是一个固定值，表示即将写入的 flash 地址。
   2. `download_path/test_bin.bin` 是一个变量，请替换为您的固件下载路径和名称。

5. 下载完成后，工具将提示 `Hash of data verified`，之后**按下 `Reset` 按键，即可进入新程序！**

   <div align="center">
   <img src="_static/linux_download.png" width="480">
   </div>

### macOS 系统更新固件说明

1. [下载最新版本固件](#下载最新版本固件)。
2. 使用 USB Type-C 数据线将开发板接入电脑，`USB-Serial-Jtag` 在 macOS 系统下无需安装驱动。
3. 安装下载工具 `esptool`，请打开 `终端` ，并输入以下指令（`pip3` 也可指定为 `pip`）：

    ```
    pip3 install esptool
    ```

   <div align="center">
   <img src="_static/macos_install_esptool.png" width="520">
   </div>

4. 请使用以下指令下载固件（`python3` 也可指定为 `python`）：

    ```
    python3 -m esptool --chip esp32s3 write_flash 0x0 download_path/test_bin.bin
    ```

   1. `0x0` 是一个固定值，表示即将写入的 flash 地址。
   2. `download_path/test_bin.bin` 是一个变量，请替换为您的固件下载路径和名称。

5. 下载完成后，工具将提示 `Hash of data verified`，之后**按下 `Reset` 按键，即可进入新程序！**

   <div align="center">
   <img src="_static/macos_download.png" width="550">
   </div>