* [English Version](./technical_architecture.md)

## 技术架构

ESP-BOX 开发框架包括系统层、框架层、应用层三个主要层级：

   <div align="center">
   <img src="_static/esp-box-tech-architecture.svg">
   </div>

ESP-BOX 仓库的代码文件结构包括 bsp 组件，例程，硬件开源资料，用户文档等，如下图：

   <div align="center">
   <img src="_static/file_structure.png" width="1000px">
   </div>

### 系统层 System Layer

ESP-IDF 是乐鑫官方开源操作系统框架，同时兼容 ESP32、ESP32-S 和 ESP32-C 系列 SoC，完成一次开发即可在多芯片平台部署。它集成了大量的系统级基础组件，用于代码编译和调试的开发工具集，以及详尽的开发指导文档，具体包括：
1. 系统级基础组件，主要包括基于 FreeRTOS 的多任务、多核支持，可外扩 PSRAM 的内存堆分配器；多款 ESP 芯片的 LL、HAL、Driver、VFS 层支持，将不同芯片功能抽象为统一操作接口；多个标准网络协议栈 TCP/IP、HTTP、MQTT、WebSocket 等等；
2. 开发工具集，主要包含用于代码编译的 GCC 交叉工具链、基于 OpenOCD 的 JTAG 调试工具、基于 Segger SystemView 的实时跟踪、Flash 和 eFuse 编程器等；
3. 开发指导文档，详尽的说明了乐鑫各个芯片平台、各个软件版本下的 API 参考，使用指引和注意事项等，开发者可[在线浏览](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)。

### 框架层 Framework Layer

除了系统层支持外，ESP-BOX 还集成了多个出色的解决方案框架，分别处理不同功能场景下的典型问题。包括用于处理语音唤醒词和命令词的语音识别框架 ESP-SR，处理显示和触摸交互功能的人机交互框架 ESP-HMI，和处理物联网功能的端到端物联网开发框架 ESP-RainMaker，接下来将简要介绍这些解决方案框架的功能和定制化方法：

**ESP-SR**

ESP-SR 是乐鑫面向智能语音领域开发的语音识别框架，该框架可利用 ESP32-S3 AI 扩展指令进行运算加速，为开发者提供开箱即用的高性能、高可靠性的语音识别功能，它包括：

1. 声学前端（[Audio Front-End, AFE](https://github.com/espressif/esp-sr/blob/3ce34fe340af15e3bfb354c21c1ec2e6e31a37e8/docs/audio_front_end/README_CN.md)）算法集，用于提升智能语音设备在远场噪声环境中的交互能力，使开发者获得高质量且稳定的音频数据。它主要包括可以有效滤除扬声器回声的回声消除算法（Acoustic Echo Cancellation，AEC）, 用于强化麦克风阵列方向性的盲源分离算法（Blind Source Separation，BSS），和用于处理环境噪声的噪声抑制算法（Noise Suppression，NS）。ESP-SR 将以上算法封装成简单的 API，开发者无需关心这些算法的具体细节，只需要针对具体的应用场景，对需要使用的算法排列组合，并按照 API 的格式要求输入待处理的语音数据，即可得到声学前端算法的处理结果。
2. 唤醒词识别模型（[WakeNet](https://github.com/espressif/esp-sr/tree/3ce34fe340af15e3bfb354c21c1ec2e6e31a37e8/docs/wake_word_engine/README.md)），用于在连续的音频流中实时监测出特定的语言片段，将智能设备从待机状态唤醒至语音交互状态。WakeNet 通过计算音频片段的梅尔倒频谱（Mel-Frequency Cepstrum, MFC）作为输入特征，使用针对 ESP32-S3 优化的神经网络算法对特征信号进行分类，在监测到命令词时触发唤醒信号，可在噪声环境下获得不小于 80% 的[有效识别率](https://github.com/espressif/esp-sr/tree/3ce34fe340af15e3bfb354c21c1ec2e6e31a37e8/docs/wake_word_engine/README.md#performance-test)。ESP-BOX 提供了开箱即用的离线唤醒词 “Hi, ESP” 和 “Hi, 乐鑫”，开发者无需额外的开发工作即可使用，同时乐鑫也支持为开发者[定制独特的唤醒词](https://github.com/espressif/esp-sr/blob/3ce34fe340af15e3bfb354c21c1ec2e6e31a37e8/docs/wake_word_engine/乐鑫语音唤醒词定制流程.md)。
3. 命令词识别模型（[MultiNet](https://github.com/espressif/esp-sr/tree/3ce34fe340af15e3bfb354c21c1ec2e6e31a37e8/docs/speech_command_recognition/README.md)），用于在设备唤醒后，识别用户特定的中英文语音命令，例如 “Turn on the air conditioner”，“关闭电灯” 等。MultiNet 通过基于 CRNN 和 CTC 的卷积循环神经网络，对输入音频片段进行多命令词识别，可同时支持最多 200 个离线命令词。 开发者不需要了解识别模型细节，也无需重新进行模型训练，通过简单配置[中文拼音字符串](https://github.com/espressif/esp-sr/tree/3ce34fe340af15e3bfb354c21c1ec2e6e31a37e8/docs/speech_command_recognition/README.md#modify-speech-commands)或[英文音标字符串](https://github.com/espressif/esp-sr/tree/3ce34fe340af15e3bfb354c21c1ec2e6e31a37e8/docs/speech_command_recognition/README.md#modify-speech-commands)以及对应 ID，即可添加或修改命令词，模型在识别到语音命令词后将触发包含 ID 信息的事件。

**ESP-HMI**

ESP-HMI 是乐鑫基于开源 GUI 框架 LVGL 设计的人机交互方案，借助 ESP32-S3 强大的计算能力和外设资源，可以实现出色的数据可视化、触摸控制等功能，它包括：
1. 屏幕驱动库：兼容多款主流 RGB、8080、SPI、I2C 接口彩色屏幕、单色屏、触摸屏，提供统一的显示驱动接口。开发者可以直接使用已适配的屏幕控制器，也可根据显示驱动接口添加自己的屏幕，UI 界面开发完成可无缝部署到多种屏幕。
2. 开源 UI 模板：乐鑫设计了适用于物联网控制面板、多媒体播放器、语音助手等不同物联网场景的多套开源 UI 模板，帮助开发者进行更专业的二次开发。
3. 硬件参考设计：提供屏幕应用相关的硬件参考设计、电路原理图和开发指南，帮助开发者快速完成硬件设计。

**ESP-RainMaker**

[ESP-RainMaker](https://rainmaker.espressif.com/docs/get-started.html) 是一个完整的物联网开发框架，作为一个端到端平台，可以为开发者提供设备配网、连云、OTA 等一站式的物联网产品功能和示例。它包含：
1. 设备端 SDK，开发者通过设计设备参数和属性，即可使用乐鑫芯片定义自己的物联网产品，快速完成设备端固件开发;
2. 透明的云中间件，开发者无需管理设备证书和云端基础设施，直接使用乐鑫芯片即可快速建立与云端的建立安全的通信链接；
3. iOS / Android 平台 APP，集成了设备发现，Wi-Fi 配网，用户登录，设备关联和控制等必要功能，开发者无需进行额外的代码开发工作。

**AI 指令集和库**

借助 ESP32-S3 Xtensa® 32­bit LX7 双核 CPU 和 AI / DSP 扩展指令集，ESP32-S3 可高效完成向量运算、复数运算、FFT 等运算，加速神经网络计算和数字信号处理计算能力。AI 开发者们可以基于乐鑫开源的基于 AI / DSP 扩展指令集的软件库，实现高性能的图像识别、语音唤醒与识别等应用。

**设备驱动**

设备驱动是框架层的底层基础，为框架层提供了控制外围设备的接口。[ESP-IoT-Solution](https://github.com/espressif/esp-iot-solution) 提供了丰富的常用外设驱动，包含传感器、显示屏、音频设备、输入设备、USB 设备等，可作为 ESP-IDF 的补充组件，方便用户实现更简单的开发。

### 应用层 Application Layer

**语音助手**

语音助手功能允许用户通过语音指令控制灯的开/关，颜色切换，打开/关闭音乐播放器，切换歌曲。更有连续语音识别，用户可以在唤醒一次设备后，与其连续对话。
