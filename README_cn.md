<p align="center">
    <img src="docs/_static/banner2.jpg" width="auto" height="auto" alt="ESPRESSIF">
</p>

* [English Version](./README.md)
* [Gitee 镜像仓库](https://gitee.com/EspressifSystems/esp-box)

# ESP-BOX AIoT 开发框架

<p align="left">
    <a href="https://github.com/espressif/esp-box/blob/master/LICENSE" alt="Build examples">
        <img alt="GitHub" src="https://img.shields.io/github/license/espressif/esp-box"></a>
    <a href="https://github.com/espressif/esp-box/actions/workflows/build_IDF5.1.yml" alt="Build examples">
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
> 对于 ESP32-S3-BOX 和 ESP32-S3-BOX-Lite，我们建议您更新内置的[出厂应用固件](./docs/firmware_update_cn.md)以确保拥有最新的 bug 修复。在 Master 分支，我们将逐步对这两款开发板提供有限的支持。有关更多信息，请阅读各例程的 README。

ESP-BOX 是乐鑫科技推出的面向 AIoT、边缘 AI 和 IIoT 应用的开发平台。ESP32-S3-BOX、ESP32-S3-BOX-Lite 和 ESP32-S3-BOX-3 是为该平台设计的系列开发板，它们基于乐鑫强大的 ESP32-S3 Wi-Fi + Bluetooth 5（LE） SoC 构建，并拥有紧凑美观的外壳结构。借助多功能配件和 ESP-BOX 项目内的可靠例程，这些开发板既适用于构建新项目原型，也适用于打造复杂的物联网系统，提供了形式上和功能上的完美平衡。

ESP-BOX 为各个领域的应用提供了开发资源，包括离线语音助手、在线 AI 聊天机器人（使用 OpenAI 和其他大型语言模型平台开发）、Matter 设备/控制器、机器人控制器、USB 主从设备、无线传感器应用以及广泛的人机交互（HMI）应用。将其加入工具箱，尽情体验各种可能性，发掘无限潜力！


| 产品名称 |        产品图      |   市场状态      |
| :-----: | :---------------------: |:---------------------: |
| [ESP32-S3-BOX](docs/hardware_overview/esp32_s3_box/hardware_overview_for_box_cn.md) | <img src="docs/_static/esp32_s3_box.png" width="200px" /> |寿命终止 <br> [用户指南](https://github.com/espressif/esp-box/blob/v0.5.0/docs/getting_started_cn.md) |
| [ESP32-S3-BOX-Lite](docs/hardware_overview/esp32_s3_box_lite/hardware_overview_for_lite_cn.md) | <img src="docs/_static/esp32_s3_box_lite.png" width="200px" /> |[在售](https://item.taobao.com/item.htm?spm=a312a.7700824.w4002-8715811646.9.4048605fNqggSF&id=658634202331) <br> [用户指南](https://github.com/espressif/esp-box/blob/v0.5.0/docs/getting_started_cn.md)|
| [ESP32-S3-BOX-3](docs/hardware_overview/esp32_s3_box_3/hardware_overview_for_box_3_cn.md) | <img src="docs/_static/esp32_s3_box_3.png" width="200px" /> |[在售](https://item.taobao.com/item.htm?ft=t&id=732842971319) <br> [用户指南](./docs/getting_started_cn.md)|


## 版本信息


|  ESP-BOX  |                        依赖的 ESP-IDF                        |                           分支信息                           | 支持状态                                                |
| :-------: | :----------------------------------------------------------: | :----------------------------------------------------------: | ------------------------------------------------------- |
|  master   |     >= release/v5.1<br/>commit id: 22cfbf30c3           | Latest developing firmware <br/>esp-sr components version:  v1.4.1 | 部分支持 ESP32-S3-BOX，ESP32-S3-BOX-Lite 和 ESP32-S3-BOX-3<br/>详见 Examples **README** |
|  Tag 0.5.0   |     release/v5.1<br/>commit id: 22cfbf30c3           | esp-sr components version:  v1.3.4 | 兼容  ESP32-S3-BOX 和 ESP32-S3-BOX-Lite<br/>使用 menuconfig 选择开发板 |
|  Tag 0.3.0   |            release/v4.4<br/>commit id: 22cfbf30c3            | esp-sr version: dev/v2.0<br/>commit id: c873a35 | 兼容  ESP32-S3-BOX 和 ESP32-S3-BOX-Lite |
| Tag 0.2.1 | release/v4.4 with [patch](https://github.com/espressif/esp-box/tree/v0.2.1/idf_patch) |      esp-sr version: close v1.0<br/>commit id: 3ce34fe       | 仅支持 ESP32-S3-BOX                                     |
| Tag 0.1.1 | Release/v4.4 with [patch](https://github.com/espressif/esp-box/tree/v0.1.1/idf_patch) |        esp-sr version: v0.9.6<br/>commit id: 3ce34fe         | 仅支持 ESP32-S3-BOX                                     |

## 支持特性

* 即开即用的入门级边缘 AI + HMI 应用开发板
* 基于 FreeRTOS 的四合一语音交互面板：离线语音识别、网络通信、屏幕显示、外设控制
* 双麦克风支持远场语音交互
* 支持高唤醒率的离线语音唤醒和命令词识别
* 允许连续命令词识别、唤醒打断以及自定义配置 200+ 中英文命令词
* 灵活多样的家庭自动化解决方案：Matter、Home Assistant、ESP-RainMaker
* 可视化拖放式 GUI 开发：LVGL SquareLine Studio、Embedded Wizard 等
* 丰富的开发框架：ESP-IDF、Arduino、PlatformIO、Circuit Python 等


## 开源内容

* [原理图与 PCB 源文件](./hardware)
* [外壳 3D 打印源文件](./hardware)
* [出厂固件源代码](./examples/factory_demo)
* [示例程序源代码](./examples)

> 基于 [Apache 2.0](https://github.com/espressif/esp-box/blob/master/LICENSE) 开源协议，你可以免费且自由的修改、创作和分享。

## 使用指引

* 首次拿到开发板，建议先阅读产品[硬件概览](./docs/hardware_overview)；
* 接着可以查看[首次使用操作指引](./docs/getting_started_cn.md#开始使用)；
* 体验有意思的[传感器应用](./docs/getting_started_cn.md#传感器监测)；
* 体验离线语音助手功能，您可以查看[离线语音识别](./docs/getting_started_cn.md#体验离线语音识别)；
* 体验更人性化的语音交互，可查看[连续语音识别](./docs/getting_started_cn.md#连续语音识别)；
* 通过手机 APP 设置独特的语音命令词，请查看 [ESP BOX APP 操作指引](./docs/getting_started_cn.md#语音命令词自定义)；
* 如需切换语音模型语言，请查看[中英文语音模型切换](./docs/getting_started_cn.md#中英文语音模型切换)；
* 了解产品爆炸图和拆机说明，请查看[拆机教程](./docs/disassembly_tutorial.md)；
* 更新最新版本固件，请查看[固件更新说明](./docs/firmware_update_cn.md)。

## 开发指引

ESP-BOX 调用乐鑫包管理器 ESP Registry 中常用的组件，您可以利用这些组件构建自己的应用。点击了解更多[开发指南](./docs/development_guide_cn.md)（持续更新中）。

### 快速开始

* **Step 1**. 如果您首次接触 ESP-IDF 开发，建议先浏览 [ESP-IDF 编程指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s3/index.html)，了解乐鑫已经提供的开源驱动和组件；
* **Step 2**. 接下来您可以详细阅读 [ESP-IDF（release/v5.1) 环境搭建指引](https://docs.espressif.com/projects/esp-idf/en/release-v5.1/esp32s3/get-started/index.html#ide)，一步一步完成开发环境搭建;
* **Step 3**. 使用指令 `git clone --recursive https://github.com/espressif/esp-box.git` 下载本项目的代码，然后需要将 esp-idf 版本切换到[指定的版本](#版本信息)；
* **Step 4**. 小试牛刀，您可以尝试 [构建并烧录一个新的示例程序](./examples/image_display)；
* **Step 5**. 更进一步，您可以阅读 [ESP-BOX 技术架构说明](./docs/technical_architecture_cn.md)，了解更多技术细节；
* **Step 6**. 深入开发，您可以阅读 [ESP 语音识别应用开发指南](https://github.com/espressif/esp-sr)，[ESP RainMaker 编程指南](https://docs.espressif.com/projects/esp-rainmaker/en/latest/)，[LVGL UI 界面开发快速总览](https://docs.lvgl.io/8.3/)；
* **Step 7**. 扩展功能，您可以阅读 [开发板 Pmod™ 兼容接口介绍](./docs/hardware_overview) 和 [ESP-IDF API 参考手册](https://docs.espressif.com/projects/esp-idf/en/release-v5.1/esp32s3/api-reference/index.html)，为更多扩展设备开发驱动。

## 联系我们

* 如果有任何开发和使用问题，您可添加 [GitHub Issues](https://github.com/espressif/esp-box/issues) 或到 [esp32.com](https://esp32.com/) 论坛寻求帮助；
* 如果你对 ESP-BOX 项目感兴趣，欢迎到论坛 [ESP-BOX 版块](https://www.esp32.com/viewforum.php?f=43)交流想法。


## 贡献示例 

我们欢迎任何软件或硬件相关的开源贡献，如果您有任何要与我们分享的示例，请参考[贡献指南](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/contribute/index.html)后提交 [Pull Requests](https://github.com/espressif/esp-box/pulls) 贡献代码。
