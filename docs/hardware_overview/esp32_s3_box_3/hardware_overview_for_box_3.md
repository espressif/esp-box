* [中文版本](./hardware_overview_for_box_3_cn.md)

## Hardware Overview

### ESP32-S3-BOX-3 
<div align="center">
<img src="./../../_static/box_3_hardware_overview/esp32_s3_box_3.png" width="300px">
</div>
ESP32-S3-BOX-3 is the main unit powered by the ESP32-S3-WROOM-1 module, which offers 2.4 GHz Wi-Fi + Bluetooth 5 (LE) wireless capability as well as AI acceleration capabilities. On top of 512 KB SRAM provided by the ESP32-S3 SoC, the module comes with additional 16 MB Quad flash and 16 MB Octal PSRAM. The board is equipped a 2.4-inch 320 x 240 SPI touch screen (the ‘red circle’ supports touch), two digital microphones, a speaker, 3‑axis Gyroscope, 3‑axis Accelerometer, one Type-C port for power and download/debug, a high-density PCIe connector which allows for hardware extensibility, as well as three functional buttons.

#### Orthographic View
<div align="center">
<img src="./../../_static/box_3_hardware_overview/esp_box_3_overview.png" width="700px">
</div>

#### Technical Specification

![specs](../../_static/box_3_hardware_overview/specs_for_box_3.png)

### ESP32-S3-BOX-3-DOCK
<div align="center">
<img src="./../../_static/box_3_hardware_overview/DOCK.jpg" width="300px">
</div>

ESP32-S3-BOX-3-DOCK is designed to serve as a stand for ESP32-S3-BOX-3 through the gold fingers, and offers diverse expandability. It has two Pmod™ compatible headers, which allow users to expand additional peripheral modules. These headers offer 16 programmable GPIOs that can supply 3.3 V power to peripherals. One USB Type-A port for connecting to diverse USB devices such as USB camera (with a maximum resolution of 720 P), USB disk, and other HID devices. While the another USB Type-C port for 5 V input power only.

#### Orthographic View
<div align="center">
<img src="./../../_static/box_3_hardware_overview/esp_box_3_dock_overview.png" width="700px">
</div>

#### Technical Specification
![specs](../../_static/box_3_hardware_overview/sepc_for_box_3_dock.png)
* [Digilent Pmod™ Interface Specification](https://digilent.com/reference/_media/reference/pmod/pmod-interface-specification-1_3_1.pdf)
  
#### Pinout Diagram
<div align="center">
<img src="./../../_static/box_3_hardware_overview/pinlayout_box_3_dock.png" width="700px">
</div>

### ESP32-S3-BOX-3-SENSOR
<div align="center">
<img src="./../../_static/box_3_hardware_overview/SENSOR.jpg" width="300px">
</div>
ESP32-S3-BOX-3-SENSOR is a versatile accessory, which integrates Temp&Hum Sensor, IR Emitter and Receiver, Radar Sensor, 18650 Rechargeable Battery slot and MicroSD Card slot. It empowers user to create wide range of innovative projects with ease. Integrate multiple sensors for detection and control, leverage the rechargeable battery for portability, and expand your storage capabilities using the MicroSD Card slot (Expand storage up to 32 GB). 

#### Orthographic View
<div align="center">
<img src="./../../_static/box_3_hardware_overview/esp_box_3_sensor_overview.png" width="700px">
</div>

#### Technical Specification
![specs](../../_static/box_3_hardware_overview/sepc_for_box_3_sensor.png)

>**Tips:**
>* When charging the 18650 battery, please switch the toggle to 'ON' and plug the ESP32-S3-BOX-3 into the ESP32-S3-BOX-3-SENSOR.

### ESP32-S3-BOX-3-BRACKET
<div align="center">
<img src="./../../_static/box_3_hardware_overview/BRACKET.jpg" width="300px">
</div>

ESP32-S3-BOX-3-BRACKET can be utilized to help ESP32-S3-BOX-3 mount to other devices, opens up a multitude of possibilities for transforming non-smart devices into smart ones. Installation of ESP32-S3-BOX-3-BRACKET is straightforward, simply prepare 2 mounting holes and a slot using our template [HERE](../../../hardware/ESP32-S3-BOX-3-BRACKET_template.PDF). By leveraging its two Pmod™ compatible headers, user can effectively integrate wireless connectivity, voice control and screen control capabilities. The ESP32-S3-BOX-3-BRACKET adapter empowers you to unleash the full potential of your non-smart devices. 
#### Orthographic View
<div align="center">
<img src="./../../_static/box_3_hardware_overview/esp_box_3_bracket_overview.png" width="700px">
</div>

#### Technical Specification
![specs](../../_static/box_3_hardware_overview/sepc_for_box_3_bracket.png)

### ESP32-S3-BOX-3-BREAD
<div align="center">
<img src="./../../_static/box_3_hardware_overview/BREAD.jpg" width="300px">
</div>
ESP32-S3-BOX-3-BREAD is an adapter that enables easy connection of the ESP32-S3-BOX-3 to a standard breadboard, providing convenience for makers who enjoy hands-on projects to expand and connect with other devices. It utilizes a high-density PCIe connector and two rows of 2.54 mm pitch pins to expose the ESP32-S3's 16 programmable GPIOs, making ESP32-S3-BOX-3-BREAD highly practical and functional.


#### Orthographic View
<div align="center">
<img src="./../../_static/box_3_hardware_overview/esp_box_3_bread_overview.png" width="700px">
</div>

#### Technical Specification
![specs](../../_static/box_3_hardware_overview/sepc_for_box_3_bread.png)

#### Pinout Diagram
<div align="center">
<img src="./../../_static/box_3_hardware_overview/pinlayout_box_3_bread.png" width="700px">
</div>

## Hardware Source Files
* [ESP32-S3-BOX-3 Kit PCB](../../../hardware/PCB_ESP32-S3-BOX-3_V1.0)
* [ESP32-S3-BOX-3 Kit Schematic](../../../hardware/SCH_ESP32-S3-BOX-3_V1.0)
* [ESP32-S3-BOX-3 3D Shell CAD](../../../hardware/esp32_s3_box_3_shell)
* [ESP32-S3-BOX-3 Bracket Mounting Template](../../../hardware/ESP32-S3-BOX-3-BRACKET_template.PDF)