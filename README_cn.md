* [English Version](./README.md)
* [Gitee 镜像仓库](https://gitee.com/EspressifSystems/esp-box)

# ESP-BOX AIoT 开发框架

<p align="left">
    <a href="https://github.com/espressif/esp-box/blob/master/LICENSE" alt="Build examples">
        <img alt="GitHub" src="https://img.shields.io/github/license/espressif/esp-box"></a>
    <a href="https://github.com/espressif/esp-box/actions/workflows/build.yml" alt="Build examples">
        <img src="https://github.com/espressif/esp-box/actions/workflows/build.yml/badge.svg" /></a>
    <a href="https://github.com/espressif/esp-box/graphs/contributors" alt="Contributors">
        <img src="https://img.shields.io/github/contributors/espressif/esp-box" /></a>
    <a href="https://github.com/espressif/esp-box/releases" alt="GitHub all releases">
        <img src="https://img.shields.io/github/downloads/espressif/esp-box/total" /></a>
</p>

<a href="https://espressif.github.io/esp-launchpad/?flashConfigURL=https://raw.githubusercontent.com/espressif/esp-box/master/launch.toml">
    <img alt="Try it with ESP Launchpad" src="https://espressif.github.io/esp-launchpad/assets/try_with_launchpad.png" width="200" height="56">
</a>

**重要提醒：**
> 我们建议您在第一次收到产品时[更新应用固件](./docs/firmware_update_cn.md)以获得最新功能和 bug 的修复，点此[链接](./docs/getting_started_cn.md)获取应用固件的使用说明。点击[此处](https://github.com/espressif/esp-box/releases)查看历史固件。

ESP-BOX 是乐鑫信息科技发布的新一代 AIoT 应用开发平台。ESP32-S3-BOX 和 ESP32-S3-BOX-Lite 是目前对应的 AIoT 应用开发板，搭载支持 AI 加速的 ESP32-S3 Wi-Fi + Bluetooth 5 (LE) SoC。他们为用户提供了一个基于语音助手 + 触摸屏控制、传感器、红外控制器和智能 Wi-Fi 网关等功能，开发和控制智能家居设备的平台。开发板出厂支持离线语音交互功能，用户通过乐鑫丰富的 SDK 和解决方案，能够轻松构建在线和离线语音助手、智能语音设备、HMI 人机交互设备、控制面板、多协议网关等多样的应用。

| 开发板名称 |        产品图        |
| :-----: | :---------------------: |
| [ESP32-S3-BOX](docs/hardware_overview/esp32_s3_box/hardware_overview_for_box_cn.md) | <img src="docs/_static/esp32_s3_box.png" width="200px" /> |
| [ESP32-S3-BOX-Lite](docs/hardware_overview/esp32_s3_box_lite/hardware_overview_for_lite_cn.md) | <img src="docs/_static/esp32_s3_box_lite.png" width="200px" /> |

## 版本信息

|  ESP-BOX  |                        依赖的 ESP-IDF                        |                           分支信息                           | 支持状态                                                |
| :-------: | :----------------------------------------------------------: | :----------------------------------------------------------: | ------------------------------------------------------- |
|  master   |               release/v5.0<br/>commit id: ef4b1b7704            | Latest developing firmware <br/>esp-sr version:  v1.1.0 | 新功能开发分支，支持  ESP32-S3-BOX |
|  Tag 0.3.0   |            release/v4.4<br/>commit id: 2bdea81b2a            | esp-sr version: dev/v2.0<br/>commit id: c873a35 | 兼容  ESP32-S3-BOX 和 ESP32-S3-BOX-Lite |
| Tag 0.2.1 | release/v4.4 with [patch](https://github.com/espressif/esp-box/tree/v0.2.1/idf_patch) |      esp-sr version: close v1.0<br/>commit id: 3ce34fe       | 仅支持 ESP32-S3-BOX                                     |
| Tag 0.1.1 | Release/v4.4 with [patch](https://github.com/espressif/esp-box/tree/v0.1.1/idf_patch) |        esp-sr version: v0.9.6<br/>commit id: 3ce34fe         | 仅支持 ESP32-S3-BOX                                     |

## 支持特性

* 双麦克风支持远场语音交互
* 高唤醒率的离线语音唤醒
* 高识别率的离线中英文命令词识别
* 可动态配置 200+ 中英文命令词
* 连续识别和唤醒打断
* 灵活可复用的 GUI 框架
* 端到端一站式接入云平台
* Pmod™ 兼容接口支持多种外设扩展

## 开源内容

* [原理图与 PCB 源文件](./hardware)
* [外壳 3D 打印源文件](./hardware)
* [出厂固件源代码](./examples/factory_demo)
* [示例程序源代码](./examples)

> 基于 [Apache 2.0](https://github.com/espressif/esp-box/blob/master/LICENSE) 开源协议，你可以免费且自由的修改、创作和分享。

## 使用指引

* 首次拿到 BOX 系列开发板，您可以查看 [首次使用操作指引](./docs/getting_started_cn.md#给设备供电)；
* 想要体验离线语音助手功能，您可以查看 [离线语音识别](./docs/getting_started_cn.md#体验离线语音识别)；
* 体验更人性化的语音交互，可查看 [连续语音识别](./docs/getting_started_cn.md#连续语音识别)；
* 想要通过手机 APP 设置独特的语音命令词，请查看 [ESP-BOX APP 操作指引](./docs/getting_started_cn.md#语音命令词自定义)；
* 想要了解产品爆炸图和拆机说明，请查看 [拆机教程](docs/disassembly_tutorial.md)；
* 想要了解开发板硬件和接口细节，请查看 [硬件总览](./docs/hardware_overview)；
* 想要使用最新版本固件，请查看 [固件更新说明](./docs/firmware_update_cn.md)。

## 开发指引

esp-box 集成了 AIOT 开发中常用的组件，您可以利用这些组件构建自己的应用。

### 快速开始

* **Step 1**. 如果您首次接触 ESP-IDF 开发，建议先浏览 [ESP-IDF 编程指南](https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32s3/index.html)，了解乐鑫已经提供的开源驱动和组件；
* **Step 2**. 接下来您可以详细阅读 [ESP-IDF（release/v4.4 或者 release/v5.0） 环境搭建指引](https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32s3/get-started/index.html#installation-step-by-step)，一步一步完成开发环境搭建;
* **Step 3**. 使用指令 `git clone --recursive https://github.com/espressif/esp-box.git` 下载本项目的代码，然后需要将 esp-idf 版本切换到[指定的版本](#版本信息)；
* **Step 4**. 小试牛刀，您可以尝试 [构建并烧录一个新的示例程序](./examples/image_display)；
* **Step 5**. 更进一步，您可以阅读 [ESP-BOX 技术架构说明](./docs/technical_architecture_cn.md)，了解更多技术细节；
* **Step 6**. 深入开发，您可以阅读 [ESP 语音识别应用开发指南](https://github.com/espressif/esp-sr)，[ESP RainMaker 编程指南](https://docs.espressif.com/projects/esp-rainmaker/en/latest/)，[LVGL UI 界面开发快速总览](https://docs.lvgl.io/8.1/get-started/index.html)；
* **Step 7**. 扩展功能，您可以阅读 [开发板 Pmod™ 兼容接口介绍](./docs/hardware_overview) 和 [ESP-IDF API 参考手册](https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32s3/api-reference/index.html)，为更多扩展设备开发驱动。

## 联系我们

* 如果有任何开发和使用问题，您可添加 [GitHub Issues](https://github.com/espressif/esp-box/issues) 或到 [esp32.com](https://esp32.com/) 论坛寻求帮助；
* 如果你对 ESP-BOX 项目感兴趣，欢迎到论坛 [ESP-BOX 版块](https://www.esp32.com/viewforum.php?f=43) 交流想法。


## 贡献示例 

我们欢迎任何软件或硬件相关的开源贡献，如果您有任何要与我们分享的示例，请参考 [贡献指南](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/contribute/index.html) 后提交  [Pull Requests](https://github.com/espressif/esp-box/pulls) 贡献代码。
