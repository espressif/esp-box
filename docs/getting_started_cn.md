* [English Version](./getting_started.md)

本用户指南适用于烧录了最新固件的 ESP32-S3-BOX、ESP32-S3-BOX-Lite 产品，下文将他们统称为 BOX 系列开发板。

# 开始使用

基于 ESP32-S3 的 BOX 系列开发板为用户提供了一个基于语音助手 + 屏幕控制、传感器、红外控制器和智能 Wi-Fi 网关等功能，开发和控制智能家居设备的平台。BOX 系列开发板出厂支持语音唤醒和离线中英文命令词识别。 其 SDK 包含可重构的 AI 语音交互功能，支持用户通过自定义命令词控制智能设备。本文的内容仅为您简要介绍最新固件能实现的参考功能，让您了解如何开始使用开发板。一旦您完成阅读下面的指南，您可以尝试自己开发应用。让我们开始旅程吧！

  
**BOX 套件包含：**

| ESP32-S3-BOX | ESP32-S3-BOX-Lite |
| :-----: | :---------------------: |
| 主机，可独立使用    | 主机，可独立使用 |
| 供测试的 RGB LED 模块和杜邦线   | 供测试的 RGB LED 模块和杜邦线  |
| Dock 配件, 辅助主机站在桌上 |  N/A|

 
**所需硬件:**
 
您需要准备一根 USB-C 数据线 （不包含在套件内）。

## 插入 RGB LED 模块 
参考下面的引脚定义，将 RGB LED 模块插入 BOX 设备。RGB LED 模块有四根公头针脚：R、G、B、GND，将它们插入 PMOD 1 对应的 G39、G40、G41、GND 母头端口。

<div align="center">
<img src="./_static/_get_started_static/hardware_pmod.png">
</div>

## 给设备供电
1. 使用一根 USB-C 数据线（不包含在套件中）给 BOX 设备供电。
<div align="center">
<img src="./_static/_get_started_static/usb_power.png" width="800px">
</div>

2. 设备启动后，屏幕上将显示 Espressif logo 的开机动画。
  
<div align="center">
<img src="./_static/_get_started_static/boot_animation.png" width="800px">
</div>

## 开始体验

1. 首先您会看几页“按键引导页”，介绍 BOX 系列开发板的按键功能，完成阅读后点击 “Next” 并进入下一页。

| ESP32-S3-BOX 按键引导页| ESP32-S3-BOX-Lite 按键引导页|
| :-----: | :---------------------: |
|  <img src="./_static/_get_started_static/0.png" width="300px"/>       | <img src="./_static/_get_started_static/1.png" width="300px"/>   |
|  <img src="./_static/_get_started_static/2.png" width="280px"/>     | <img src="./_static/_get_started_static/2.png" width="280px"/>   |

2. 接下来是“语音助手使用说明”引导页 ，请仔细阅读，它会提示您如何使用 AI 语音功能。完成阅读后点击 “OK Let's Go” 进入菜单页。
<div align="center">
<img src="./_static/_get_started_static/3_CN.png" > 
</div>
<div align="center">
<img src="./_static/_get_started_static/3.1_CN.png"> 
</div>
3. 菜单页有五大功能，分别是“设备控制”、“音乐播放器”、“网络”、“帮助”以及“关于”，左右切换可进入不同的功能。比如，进入“设备控制”用户界面，选中 “Light” 图标，点按确认可打开或关闭 LED 灯。然后回到菜单页，找到“音乐播放器”界面并进入，进行歌曲的播放或调节音量。
<div align="center">
<img src="./_static/_get_started_static/4.png">
</div>
<div align="center">
<img src="./_static/_get_started_static/4.1.png">
</div>
<div align="center">
<img src="./_static/_get_started_static/5.png">
</div>
<div align="center">
<img src="./_static/_get_started_static/5.1.png">
</div>

**以下功能仅 ESP32-S3-BOX 支持**

4. 按下 ESP32-S3-BOX 顶部的静音按钮，按下后设备语音唤醒和识别功能被禁用，再次按下恢复语音功能。
   <div align="center">
   <img src="_static/_get_started_static/hardware_mute_button.jpg" width="350px">
   </div>

5. 也可以轻触触摸屏下方的 “小红圈” 返回上一页。
   <div align="center">
   <img src="./_static/_get_started_static/hardware_home.png" width="350px">

## 体验离线语音识别
1. 在任意界面，您可以使用唤醒词去唤醒设备，中文唤醒词是“Hi 乐鑫”，英文唤醒词是 “Hi E. S. P.”（用字母发音）。设备唤醒成功后，唤醒词将显示在如下屏幕上并伴有提示音，如果唤醒词未出现，请再次尝试。如下展示的动画界面提示您的设备正在倾听。

<div align="center">
<img src="./_static/_get_started_static/16.png">
</div>


2. 请在提示音响起后的 6 秒左右时间内对设备说命令词，比如“打开电灯”，同时命令词将显示在屏幕上并且 LED 模块打开。约 1 秒后语音控制界面退出。
<div align="center">
<img src="./_static/_get_started_static/20.png">
</div>
   
3. 您也可以使用语音命令欣赏音乐，请先唤醒设备然后对设备说命令词“播放音乐”，设备会打开音乐播放器并开始播放内置音乐，您也可以用语音命令词切换下一首歌或者暂停播放。（最新固件默认自带两首歌）
   
   >**注意：**
   >* 如果 LED 灯没有被打开，请检查 RGB 模块的 pin 脚是否插错。
   >* 若在规定时间内未识别到命令词，则显示超时，约 1 秒后界面关闭。
   
<div align="center">
<img src="_static/_get_started_static/19.png">
</div>
   
4. 默认命令词为："**打开电灯**", "**关闭电灯**", "**调成红色**", "**调成绿色**", "**调成蓝色**", "**播放音乐**", "**切歌**", "**暂停播放**"；

## 语音命令词自定义
BOX 系列开发板配备乐鑫专有的 AI 语音识别系统，您可以通过 “ESP-BOX” APP 自定义用于指令的任意命令词，我们将以 LED 灯作为示例介绍如何自定义语音命令词。（有关算法工作原理的详细信息，请参阅开发指引里的技术架构说明。）

**1. 连接 ESP-BOX 手机 APP**

1.1. 首先找到“网络”部分，点击确认后进入如下设备配网界面，在此之前，您需要安装 ESP-BOX APP。点击右上角 “To install APP” 部分下载并安装 ESP-BOX APP。

   <div align="center">
   <img src="_static/_get_started_static/6.png">
   </div>

1.2. 扫描如下二维码下载并安装 ESP-BOX APP，您也可以直接在 Apple Store 和 Google Play 搜索“ESP-BOX”下载 APP。

   <div align="center">
   <img src="_static/_get_started_static/8.png">
   </div>
   <div align="center">
   <img src="_static/_get_started_static/Picture1.png">
   </div>
1.3. 如您第一次下载使用，请点击  “User”，“Register”，注册一个新账户。
   
1.4. 登录您的 ESP-BOX 账号并打开手机蓝牙。然后找到界面下方的“+” ，点击扫码图标扫描设备上的配网二维码。

   <div align="center">
   <img src="_static/_get_started_static/1.jpeg"width="300" />
   </div>
   <div align="center">
   <img src="_static/_get_started_static/2.jpeg"width="300" />
   </div>
   <div align="center">
   <img src="_static/_get_started_static/3.jpeg"width="300" />
   </div>
1.5 请按 APP 内指引添加您的设备，设备添加完成后，您将看到如下界面:
 
   <div align="center">
   <img src="_static/_get_started_static/4.jpeg"width="300" />
   </div>
   <div align="center">
   <img src="_static/_get_started_static/5.jpeg"width="300" />
   </div>
   <div align="center">
   <img src="_static/_get_started_static/7.png">
   </div>
   
>**注意：**

   >* 确保设备连接的网络是 2.4GHz Wi-Fi 而不是 5GHz，并且输入正确的 Wi-Fi 密码，如果 Wi-Fi 密码错误，会显示 “Wi-Fi Authentication failed”。
   >* 在配网过程中不要退出设备配网界面。
   >* 长按 Boot 按键 5 秒可清除配网信息恢复出厂设置。
   <div align="center">
   <img src="_static/_get_started_static/7.jpeg"width="300" />
   </div>
   
   **2. 自定义语音命令词：**

2.1. 点击你的 ESP-BOX 设备，进入如下页面，您可以简单地在这个用户界面开灯或者关灯。（我们预留了“开关”和“风扇”功能给您自行去配置和开发。）

   <div align="center">
   <img src="_static/_get_started_static/8.jpeg"width="300" />
   </div>
   <div align="center">
   <img src="_static/_get_started_static/9.jpeg"width="300" />
   </div>
2.2. 点按 “Light”，进入 “Configure” 页面查看当前默认的 pin 脚信息和命令词。您也可以根据需要修改 LED 模块的 “RED”、“GREEN” 和 “BLUE” pin 脚。

   <div align="center">
   <img src="_static/_get_started_static/10 CN.jpeg"width="300" />
   </div>
   <div align="center">
   <img src="_static/_get_started_static/11.jpeg"width="300" />
   </div>
   
2.3. “Control” 页面允许您对灯光进行颜色、亮度和饱和度的更改。
  
   <div align="center">
   <img src="_static/_get_started_static/12.jpeg"width="300" />
   </div>
   
2.4. “Configure” 页面允许您针对设备的“开”、“关”、“颜色”定义您喜欢的命令词来控制设备。比如，您可以自定义“开”的命令词为“早上好”（如下图标注“1”步骤），点击保存，界面返回上一级（如下图标注“2”步骤），然后再点击保存（如下图标注“3”步骤）。

   <div align="center">
   <img src="_static/_get_started_static/13 CN.jpeg"width="300" />
   </div>
   <div align="center">
   <img src="_static/_get_started_static/13.1 CN.jpeg"width="300" />
   </div>
   
2.5. 现在，您可以体验您的新命令词了！请先使用 “Hi 乐鑫” 去唤醒设备，然后在六秒内说出打开电灯的新命令词“早上好”，新命令词会显示在屏幕上并且 LED 模块打开。
   <div align="center">
   <img src="_static/_get_started_static/14.png"width="300" />
   </div>
   
>**如何添加合适的命令词：AI 模型支持自定义命令词，为了获得最优的识别体验，命令词定义时需要注意以下事项：**

>* 命令词的长度：命令词中单词数需要大于等于 2 个，小于等于 8 个。在定义一系列命令词时，最好可以保持不同命令词的长度相似。
>* 避免前缀：多个命令词，相互之间不要是对方的前缀，短的词会被屏蔽，比如 “打开”和“打开灯”，“打开”会被屏蔽。


以上指南目的是让您简要了解如何在 BOX 系列开发板上使用最新的固件，其他三个功能图标 (Switch/Fan/Air) 仅用于展示，暂未开发。您可以开始尝试编写令人兴奋的应用程序，开启您的物联网之旅！

