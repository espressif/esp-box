
# JoyStick-Controller Example

[[English]](README.md)

| 开发板             | 支持状态        |
| ----------------- | -------------- |
| ESP32-S3-BOX      | NO             |
| ESP32-S3-BOX-Lite | NO             |
| ESP32-S3-BOX-3    | YES            |

该项目基于乐鑫的 ESP32-S3-BOX-3 和 ESP32-S3-USB-Bridge 进行开发，旨在创建一个兼顾游戏手柄和航模遥控器的开源项目。

本指南将帮助您快速上手 ESP-JoyStick，并提供该项目的所有详细信息。更多信息请参见 [ESP32-S3-BOX-3-JoyStick](https://oshwhub.com/esp-college/esp_box_3_joystick)。

本指南包括以下部分：
- 项目概述
- 硬件参考
- 应用程序开发
- 相关文档

## 项目概述

ESP-JoyStick 硬件系统由 ESP32-S3-BOX-3 开发板、 JoyStick 手柄和 ESP32-S3-USB-Bridge 接收机三部分组成。ESP32-S3-BOX-3 作为主控制器，通过 PCIe 接口与 JoyStick 手柄进行连接。系统在“游戏模式”和“遥控模式”下运行。在游戏模式下，它支持 USB-HID 和 BLE-HID 协议，用于控制计算机游戏，以及 NES 模拟器模式。在 RC 遥控模式下，它利用 ESP-NOW 无线通信协议与 [ESP32-S3-USB-Bridge](https://docs.espressif.com/projects/espressif-esp-dev-kits/zh_CN/latest/esp32s3/esp32-s3-usb-bridge/user_guide.html)（购买链接：[淘宝](https://item.taobao.com/item.htm?ft=t&id=753321694177)或[速卖通](https://www.aliexpress.us/item/3256806114330511.html?gatewayAdapt=glo2usa4itemAdapt)）或其他 Espressif 开发板（这里的 ESP32-S3-USB-Bridge 或其他 Espressif 开发板作为航模接收机）进行配对，实现 [RC 遥控车](../joystick_rc_receiver)和 [ESP-Drone](https://docs.espressif.com/projects/espressif-esp-drone/en/latest/gettingstarted.html) 四轴飞行器的无线控制。

<div align="center">
<img src="https://dl.espressif.com/ae/esp-box/ESP32-S3-BOX-3-JoyStick-overall.png/ESP32-S3-BOX-3-JoyStick-overall.png" width="60%">
<p>ESP-JoyStick 整体实物图</p>
</div>

- 游戏模式：
  - USB-HID 模式：使用 Type-C 数据线连接电脑，通过 USB-HID 协议控制电脑游戏，可自定义摇杆及按键功能。
  - BLE-HID 模式：该模式下 JoyStick 与电脑通过蓝牙进行连接，通过 BLE-HID 协议控制电脑游戏，可自定义摇杆及按键功能。
  - NES 模拟器模式：该模式下，可在 ESP32-S3-BOX-3 屏幕上显示 NES 模拟器中的游戏。

<div align="center">
<img src="https://dl.espressif.com/ae/esp-box/control_pc_games.gif/control_pc_games.gif" width="60%">
<p>ESP-JoyStick 控制电脑游戏</p>
</div>

- RC 遥控模式：ESP-JoyStick 通过 ESP-NOW 无线通信协议与 [ESP32-S3-USB-Bridge](https://docs.espressif.com/projects/espressif-esp-dev-kits/zh_CN/latest/esp32s3/esp32-s3-usb-bridge/user_guide.html) 等其他乐鑫官方开发板（接收机）进行配对连接，可实现对 [RC 遥控车](../joystick_rc_receiver)、[ESP-Drone](https://docs.espressif.com/projects/espressif-esp-drone/en/latest/gettingstarted.html) 四旋翼小飞机等航模的无线遥控。

<div align="center">
<img src="https://dl.espressif.com/ae/esp-box/control_esp_drone.gif/control_esp_drone.gif" width="60%">
<p>ESP-JoyStick 遥控 ESP-Drone 小飞机</p>
</div>

<div align="center">
<img src="https://dl.espressif.com/ae/esp-box/control_rc_car.gif/control_rc_car.gif" width="60%">
<p>ESP-JoyStick 控制 RC 遥控车</p>
</div>

## 硬件参考

为保证 ESP-JoyStick 具有较好的握持手感，本项目对 ESP-JoyStick 的 PCB 板形进行了优化设计，并为其设计了配套的 [3D 外壳](https://dl.espressif.com/ae/esp-box/Box_3_JoyStick_Case_231116.STL/Box_3_JoyStick_Case_231116.STL)。

以下是 ESP-JoyStick 的主要部件分布图。

<div align="center">
<img src="https://dl.espressif.com/ae/esp-box/ESP32-S3-BOX-3-JoyStick-front_cn.png/ESP32-S3-BOX-3-JoyStick-front_cn.png" width="60%">
<p>ESP-JoyStick 正面</p>
</div>

<div align="center">
<img src="https://dl.espressif.com/ae/esp-box/ESP32-S3-BOX-3-JoyStick-back_cn.png/ESP32-S3-BOX-3-JoyStick-back_cn.png" width="60%">
<p>ESP-JoyStick 背面</p>
</div>

## 应用程序开发

ESP-JoyStick 共有两个摇杆，每个摇杆（其原理类似于可调电阻，摇杆在不同位置的电阻值不同，利用串联分压原理，通过检测得出的电压值即可推算出摇杆位置）需要使用两路 ADC 进行姿态读取，除此之外，摇杆内部还有一个薄膜按键可供开发者使用。

ESP-JoyStick 共有 16 个物理按键，每个按键“未按下时电平为高电平，按下为低电平”，由于 ESP32-S3-BOX-3 引出的 IO 口数量有限，因此该项目使用两个 74HC165D 并行转串行芯片，开发者通过三个 IO 口模拟 SPI 通信即可读取到所有的按键状态。

震动马达通过一个三极管（S8050）进行控制，开发者只需将一个 IO 口配置为输出模式，输出高电平马达震动，输出低电平马达不震动。

具体的引脚分配请查看 ESP-JoyStick 原理图。

## 相关文档

* 原理图和 PCB 文件：

  * [JoyStick_MainBoard_Schematic](https://dl.espressif.com/ae/esp-box/SCH_JoyStick_MainBoard_Schematic_2023-11-22.pdf/SCH_JoyStick_MainBoard_Schematic_2023-11-22.pdf)
  * [PCIE_Board_Schematic](https://dl.espressif.com/ae/esp-box/SCH_PCIE_Board_Schematic_2023-11-22.pdf/SCH_PCIE_Board_Schematic_2023-11-22.pdf)
  * [ButtonBoard_Schematic](https://dl.espressif.com/ae/esp-box/SCH_ButtonBoard_Schematic_2023-11-22.pdf/SCH_ButtonBoard_Schematic_2023-11-22.pdf)
  * [JoyStick_MainBoard_PCB](https://dl.espressif.com/ae/esp-box/PCB_JoyStick_MainBoard_PCB_2023-11-22.pdf/PCB_JoyStick_MainBoard_PCB_2023-11-22.pdf)
  * [PCIE_Board_PCB](https://dl.espressif.com/ae/esp-box/PCB_PCIE_Board_PCB_2023-11-22.pdf/PCB_PCIE_Board_PCB_2023-11-22.pdf)
  * [ButtonBoard_PCB](https://dl.espressif.com/ae/esp-box/PCB_ButtonBoard_PCB_2023-11-22.pdf/PCB_ButtonBoard_PCB_2023-11-22.pdf)

* [外壳 3D 打印文件](https://dl.espressif.com/ae/esp-box/Box_3_JoyStick_Case_231116.STL/Box_3_JoyStick_Case_231116.STL)


