* [English Version](./README.md)
* [Gitee 镜像仓库](https://gitee.com/EspressifSystems/esp-box)

# ESP-BOX AIoT 开发框架

**重要提醒：**
**我们建议您在第一次收到产品时[更新 ESP32-S3-BOX 的固件](./docs/firmware_update_cn.md)以获得最新功能和 bug 的修复。**

ESP-BOX 是乐鑫信息科技发布的新一代 AIoT 应用开发平台。ESP32-S3-BOX 是对应的 AIoT 应用开发板，搭载支持 AI 加速的 ESP32-S3 Wi-Fi + Bluetooth 5 (LE) SoC。ESP32-S3-BOX 为用户提供了一个基于语音助手 + 触摸屏控制、传感器、红外控制器和智能 Wi-Fi 网关等功能，开发和控制智能家居设备的平台。ESP32-S3-BOX 出厂支持离线语音交互功能，通过乐鑫丰富的 SDK 和解决方案，能够轻松构建在线和离线语音助手、智能语音设备、HMI 触摸屏设备、控制面板、多协议网关等多样的应用。

![esp_box_hardware](./docs/_static/esp32_s3_box_hardware.svg)

**ESP-BOX 支持以下特性:**

* 双麦克风支持远场语音交互
* 高唤醒率的离线语音唤醒
* 高识别率的离线中英文命令词识别
* 可动态配置的中英文命令词
* 灵活可复用的 GUI 框架
* 端到端一站式接入云平台
* Pmod™ 兼容接口支持多种外设扩展

**ESP-BOX 开源内容包括:**

* [原理图与 PCB 源文件](./hardware)
* [外壳 3D 打印源文件](./hardware/esp32_s3_box_shell_step)
* [出厂固件源代码](./examples/factory_demo)
* [示例程序源代码](./examples)

> 基于 [Apache 2.0](https://github.com/espressif/esp-box/blob/master/LICENSE) 开源协议，你可以免费且自由的修改、创作和分享。

## 使用指引

* 首次拿到 ESP32-S3-BOX，您可以查看[首次使用操作指引](./docs/getting_started_cn.md#给-esp32-s3-box-供电)
* 想要设置独特的语音命令词，您可以查看[修改自定义命令词](./docs/getting_started_cn.md#语音控制命令词自定义)
* 想要通过界面控制设备，请查看[图形界面操作指引](./docs/getting_started_cn.md#esp32-s3-box-图形界面)
* 想要了解产品爆炸图和拆机说明, 请查看 [ESP32-S3-BOX 拆机教程](docs/disassembly_tutorial.md)
* 想要了解 ESP32-S3-BOX 硬件和接口细节，请查看 [ESP32-S3-BOX 硬件总览](./docs/hardware_overview_cn.md)
* 想要使用最新版本固件，请查看[固件更新说明](./docs/firmware_update_cn.md)

## 开发指引

* **Step 1**. 如果您首次接触 ESP-IDF 开发，建议先浏览 [ESP-IDF 编程指南](https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32s3/index.html)，了解乐鑫已经提供的开源驱动和组件；
* **Step 2**. 接下来您可以详细阅读 [ESP-IDF（release/v4.4） 环境搭建指引](https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32s3/get-started/index.html#installation-step-by-step), 一步一步完成开发环境搭建；
* **Step 3**. 此外, 请在当前 ESP-IDF 上打上补丁, 详情请参考 [Patch for ESP-IDF](./idf_patch).
* **Step 4**. 小试牛刀，您可以尝试 [构建并烧录一个新的示例程序](./examples/image_display)；
* **Step 5**. 更进一步，您可以阅读 [ESP-BOX 技术架构说明](./docs/technical_architecture_cn.md)，了解更多技术细节；
* **Step 6**. 深入开发，您可以阅读 [ESP 语音识别应用开发指南](https://github.com/espressif/esp-sr), [ESP RainMaker 编程指南](https://docs.espressif.com/projects/esp-rainmaker/en/latest/), [LVGL UI 界面开发快速总览](https://docs.lvgl.io/latest/en/html/get-started/quick-overview.html)；
* **Step 7**. 扩展功能，您可以阅读 [ESP32-S3-BOX Pmod™ 兼容接口介绍](./docs/hardware_overview_cn.md) 和 [ESP-IDF API 参考手册](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/index.html)，为更多扩展设备开发驱动；

## 联系我们

* 如果有任何开发和使用问题，您可添加 [GitHub Issues](https://github.com/espressif/esp-box/issues) 或到 [esp32.com](https://esp32.com/) 论坛寻求帮助；
* 如果你对 ESP-BOX 项目感兴趣，欢迎到论坛 [ESP-BOX 版块](https://esp32.com/viewforum.php?f=43)交流想法；
* 我们欢迎任何软件或硬件相关的开源贡献，您可直接提交 [Pull Requests](https://github.com/espressif/esp-box/pulls) 贡献代码。
