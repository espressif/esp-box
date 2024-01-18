
# JoyStick-Controller Example

[[中文]](README_cn.md)

| Board             | Support Status |
| ----------------- | -------------- |
| ESP32-S3-BOX      | NO             |
| ESP32-S3-BOX-Lite | NO             |
| ESP32-S3-BOX-3    | YES            |

This project is developed based on Espressif's ESP32-S3-BOX-3 and ESP32-S3-USB-Bridge, aiming to create an open-source project that caters to both gaming controllers and remote control for model aircraft.

This guide will assist you in quickly getting started with ESP-JoyStick and provide all the detailed information about the project.

More information is on [ESP32-S3-BOX-3-JoyStick](https://oshwhub.com/esp-college/esp_box_3_joystick).

This guide includes the following sections:
- Project Overview
- Hardware Reference
- Application Development
- Related Documentation

## Project Overview

The ESP-JoyStick hardware system comprises the ESP32-S3-BOX-3 development board, JoyStick controller, and ESP32-S3-USB-Bridge receiver. The ESP32-S3-BOX-3 serves as the main controller, connecting to the JoyStick controller and ESP32-S3-USB-Bridge receiver through a PCIe interface. The system operates in "Game Mode" and "RC Remote Control Mode." In Game Mode, it supports USB-HID and BLE-HID protocols for computer gaming control, as well as an NES emulator mode. In RC Remote Control Mode, it utilizes the ESP-NOW wireless communication protocol to pair with [ESP32-S3-USB-Bridge](https://docs.espressif.com/projects/espressif-esp-dev-kits/en/latest/esp32s3/esp32-s3-usb-bridge/user_guide.html)(Purchase link: [TaoBao](https://item.taobao.com/item.htm?ft=t&id=753321694177) or [Aliexpress](https://www.aliexpress.us/item/3256806114330511.html?gatewayAdapt=glo2usa4itemAdapt)) and other Espressif development boards(where the ESP32-S3-USB-Bridge or other Espressif development boards act as the model aircraft receiver), enabling wireless control of [RC cars](../joystick_rc_receiver) and [ESP-Drone](https://docs.espressif.com/projects/espressif-esp-drone/en/latest/gettingstarted.html) quadcopters.

<div align="center">
<img src="https://dl.espressif.com/ae/esp-box/ESP32-S3-BOX-3-JoyStick-overall.png/ESP32-S3-BOX-3-JoyStick-overall.png" width="60%">
<p>ESP-JoyStick Overall Physical Diagram</p>
</div>

- Game Mode:
  - USB-HID Mode: Connect to the computer using a Type-C data cable, control computer games through the USB-HID protocol, and customize joystick and button functions.
  - BLE-HID Mode: In this mode, the JoyStick is connected to the computer via Bluetooth using the BLE-HID protocol, enabling control of computer games with customizable joystick and button functions.
  - NES Emulator Mode: In this mode, games from the NES emulator can be displayed on the screen of ESP32-S3-BOX-3.

<div align="center">
<img src="https://dl.espressif.com/ae/esp-box/control_pc_games.gif/control_pc_games.gif" width="60%">
<p>ESP-JoyStick control computer games</p>
</div>

- RC Remote Control Mode: ESP-JoyStick establishes a paired connection through the ESP-NOW wireless communication protocol with other Espressif official development boards (receivers) such as [ESP32-S3-USB-Bridge](https://docs.espressif.com/projects/espressif-esp-dev-kits/zh_CN/latest/esp32s3/esp32-s3-usb-bridge/user_guide.html). This enables wireless remote control for [RC cars](../joystick_rc_receiver), [ESP-Drone](https://docs.espressif.com/projects/espressif-esp-drone/en/latest/gettingstarted.html) quadcopters, and other model aircraft.

<div align="center">
<img src="https://dl.espressif.com/ae/esp-box/control_esp_drone.gif/control_esp_drone.gif" width="60%">
<p>ESP-JoyStick remote control the ESP-Drone</p>
</div>

<div align="center">
<img src="https://dl.espressif.com/ae/esp-box/control_rc_car.gif/control_rc_car.gif" width="60%">
<p>ESP-JoyStick control the RC car</p>
</div>

## Hardware Reference

To ensure that ESP-JoyStick has a comfortable grip, this project has optimized the PCB board shape and designed a matching [3D enclosure](https://dl.espressif.com/ae/esp-box/Box_3_JoyStick_Case_231116.STL/Box_3_JoyStick_Case_231116.STL) for it.

  The following is a map of the main components of the ESP-JoyStick.

<div align="center">
<img src="https://dl.espressif.com/ae/esp-box/ESP32-S3-BOX-3-JoyStick-front_en.png/ESP32-S3-BOX-3-JoyStick-front_en.png" width="60%">
<p>ESP-JoyStick Front</p>
</div>

<div align="center">
<img src="https://dl.espressif.com/ae/esp-box/ESP32-S3-BOX-3-JoyStick-back_en.png/ESP32-S3-BOX-3-JoyStick-back_en.png" width="60%">
<p>ESP-JoyStick Back</p>
</div>

## Application Development

ESP-JoyStick has two joysticks. Each joystick, functioning similarly to a variable resistor, has varying resistance values at different positions. Utilizing the series voltage divider principle, the joystick's position can be inferred by detecting the voltage value. To capture the attitude, two ADC channels are used for each joystick. In addition to this, each joystick has a membrane button that developers can utilize.

ESP-JoyStick has a total of 16 physical buttons. Each button operates with a high logic level when not pressed and a low logic level when pressed. Due to the limited number of IO ports available on ESP32-S3-BOX-3, this project employs two 74HC165D parallel-to-serial shift registers. Developers can simulate SPI communication using three IO ports to read the status of all the buttons.

The vibration motor is controlled through a transistor (S8050). Developers simply need to configure one IO port as an output mode. When the port outputs a high logic level, the motor vibrates, and when it outputs a low logic level, the motor does not vibrate.

For specific pin assignments, please refer to the schematic diagram of ESP-JoyStick.

## Related Documentation

* Schematic and PCB Source File:

  * [JoyStick_MainBoard_Schematic](https://dl.espressif.com/ae/esp-box/SCH_JoyStick_MainBoard_Schematic_2023-11-22.pdf/SCH_JoyStick_MainBoard_Schematic_2023-11-22.pdf)
  * [PCIE_Board_Schematic](https://dl.espressif.com/ae/esp-box/SCH_PCIE_Board_Schematic_2023-11-22.pdf/SCH_PCIE_Board_Schematic_2023-11-22.pdf)
  * [ButtonBoard_Schematic](https://dl.espressif.com/ae/esp-box/SCH_ButtonBoard_Schematic_2023-11-22.pdf/SCH_ButtonBoard_Schematic_2023-11-22.pdf)
  * [JoyStick_MainBoard_PCB](https://dl.espressif.com/ae/esp-box/PCB_JoyStick_MainBoard_PCB_2023-11-22.pdf/PCB_JoyStick_MainBoard_PCB_2023-11-22.pdf)
  * [PCIE_Board_PCB](https://dl.espressif.com/ae/esp-box/PCB_PCIE_Board_PCB_2023-11-22.pdf/PCB_PCIE_Board_PCB_2023-11-22.pdf)
  * [ButtonBoard_PCB](https://dl.espressif.com/ae/esp-box/PCB_ButtonBoard_PCB_2023-11-22.pdf/PCB_ButtonBoard_PCB_2023-11-22.pdf)
* [Shell 3D Print Source File](https://dl.espressif.com/ae/esp-box/Box_3_JoyStick_Case_231116.STL/Box_3_JoyStick_Case_231116.STL)


