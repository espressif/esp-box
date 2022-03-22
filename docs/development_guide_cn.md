# 开发指南

本文档旨在对用户从设置开发环境到应用开发提供帮助，以便使用基于 Espressif 的 ESP32-S3 芯片进行 AIoT 应用开发。

## 准备

- 开发环境的安装

  按照 [ESP-IDF 编程指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s3/get-started/index.html#get-started-get-prerequisites)的指引完成 esp-idf 开发环境的安装，尽管有图形化的IDE和插件，但是我们推荐您手动安装并使用命令提示行进行编译。

  按照以上指引完成的安装很可能并不是编译 esp-box 依赖的 esp-idf 版本，此时需要根据[版本说明](https://github.com/espressif/esp-box#versions)来切换 esp-idf 到指定的版本。以 commit id 为 2bdea81b2a 为例子，下面的命令将完成切换

  ```shell
  cd esp-idf
  git fetch
  git checkout 2bdea81b2a
  git submodule update --init --recursive
  ./install.sh
  ```

- 获取 esp-box

  在命令提示行中进入到自己工作的目录后运行如下命令克隆仓库

  ```shell
  git clone --recursive https://github.com/espressif/esp-box.git
  ```

## 编译并运行第一个应用

在成功完成准备工作后就可以开始编译第一个应用了，你可以从 `examples/factory_demo` 工程开始。

### 设置 esp-idf 环境变量

每打开一个新的命令提示行进行编译都需要这一步

```shell
cd esp-idf
. ./export.sh
```

### 进行编译

进入到工程目录下后编译

```shell
cd esp-box/examples/factory_demo
idf.py build
```

您将看到如下的 log 输出：

```
Executing action: all (aliases: build)
Running cmake in directory /home/user/esp-box/examples/factory_demo/build
Executing "cmake -G Ninja -DPYTHON_DEPS_CHECKED=1 -DESP_PLATFORM=1 -DIDF_TARGET=esp32s3 -DCCACHE_ENABLE=0 /home/user/work/esp-box/examples/factory_demo"...
-- Found Git: /usr/bin/git (found version "2.36.0") 
-- Not find RMAKER_PATH, default is /home/user/work/esp-box/examples/factory_demo/../../components/esp-rainmaker
-- Component directory /home/user/work/esp-box/components/esp-rainmaker does not contain a CMakeLists.txt file. No component will be added
-- Component directory /home/user/work/esp-box/components/esp-rainmaker/components/esp-insights does not contain a CMakeLists.txt file. No component will be added
-- The C compiler identification is GNU 8.4.0
-- The CXX compiler identification is GNU 8.4.0
-- The ASM compiler identification is GNU
-- Found assembler: /home/user/esp/.espressif/tools/xtensa-esp32s3-elf/esp-2021r2-8.4.0/xtensa-esp32s3-elf/bin/xtensa-esp32s3-elf-gcc

......

Project build complete. To flash, run this command:
/home/user/esp/.espressif/python_env/idf4.4_py3.8_env/bin/python ../../../../esp/esp-idf/components/esptool_py/esptool/esptool.py -p (PORT) -b 460800 --before default_reset --after hard_reset --chip esp32s3 --no-stub write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0x16000 build/ota_data_initial.bin 0x20000 build/factory_demo.bin 0x3bd000 build/storage.bin 0x647000 build/model.bin
```

### 烧录固件并运行

BOX 开发板均可通过 USB 接口直接下载固件。在下载之前需要确保电脑正确识别了设备

- Linux 和 MacOS 通常无需安装驱动即可识别。
- 对于 Windos 系统的电脑，我们推荐使用 `Windows 10` 及以上版本，在该系统下 `USB-Serial-Jtag` 的驱动将联网自动下载。如果使用 `Windows 7` 系统，请手动下载 [USB-Serial-JTAG 驱动](https://dl.espressif.com/dl/idf-driver/idf-driver-esp32-usb-jtag-2021-07-15.zip) 并安装。

通过以下命令下载固件并打开监视器：

```shell
idf.py -p PORT flash monitor
```

请替换 PORT 为电脑识别到的端口名称，Linux 系统下通常为 `/dev/ttyACM0`

下载固件完成将会自动开始运行

## 调试应用程序

- JTAG 调试 参见：https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s3/api-guides/jtag-debugging/index.html
- 应用层跟踪 参见：https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s3/api-guides/app_trace.html

## 组件

包含以下组件：


| 组件          | 说明                                   |
| ------------- | -------------------------------------- |
| bsp           | 包含已支持开发板信息以及板载外设的驱动 |
| esp-rainmaker | 与 Rainmaker 云连接                    |
| esp-sr        | 乐鑫语音识别库                         |
| i2c_bus       | I2C 驱动                               |
| i2c_devices   | 一些常用 I2C 设备驱动                  |
| iot_button    | 按键驱动                               |
| libhelix-mp3  | MP3 解码库                             |
| lvgl          | LVGL 图形库                            |



### bsp 组件





