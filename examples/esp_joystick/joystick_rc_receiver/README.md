# JoyStick-RC-Receiver Example

[[中文]](README_cn.md)

This example represents the model aircraft receiver functionality module for the [ESP-JoyStick](../joystick_controller) project. Using this sample program, you can control an RC remote control car with the following effects:

* Upon successful pairing and connection with ESP-JoyStick through ESP-NOW, the RGB indicator light turns green for 1 second and then turns off. When not paired, the RGB indicator light remains solid red.
* Pushing the left joystick up/down makes the car move forward/backward.
* Pushing the left joystick down makes the car move backward, and the yellow reverse light turns on.
* Pushing the right joystick left/right makes the car turn left/right.
* Pressing button A activates the car's brakes, and the red brake light turns on.
* Pressing LB/RB flashes the left/right turn signal lights. Pressing LB/RB again or pushing the right joystick left/right turns off the left/right turn signal lights.

<div align="center">
<img src="https://dl.espressif.com/ae/esp-box/control_rc_car.gif/control_rc_car.gif" width="60%">
<p>ESP-JoyStick control the RC car</p>
</div>

## Hardware

This example program is developed based on the [ESP32-S3-USB-Bridge](https://docs.espressif.com/projects/espressif-esp-dev-kits/en/latest/esp32s3/esp32-s3-usb-bridge/user_guide.html) development board(Purchase link: [TaoBao](https://item.taobao.com/item.htm?ft=t&id=753321694177) or [Aliexpress](https://www.aliexpress.us/item/3256806114330511.html?gatewayAdapt=glo2usa4itemAdapt)), and the specific pin assignments are as follows:
| Pin             | Function Description |
| ----------------- | -------------- |
| GPIO42   | ESP-NOW connection status indicator LED pin  |
| GPIO2    | Forward PWM control pin for the car |
| GPIO3    | Backward PWM control pin for the car |
| GPIO4    | Car steering servo PWM signal control pin |
| GPIO8    | Turn left signal light control pin, high level for on, low level for off |
| GPIO9    | Turn right signal light control pin, high level for on, low level for off |
| GPIO5    | Brake light control pin, high level for on, low level for off |
| GPIO40   | Reverse light control pin, high level for on, low level for off |

## How to use example

### Build and Flash

Run `idf.py flash monitor` to build, flash and monitor the project.

### Example Output

Run this example, you will see the following output log:

```
I (391) app_start: Starting scheduler on CPU0
I (395) app_start: Starting scheduler on CPU1
I (395) main_task: Started on CPU0
I (405) main_task: Calling app_main()
I (409) gpio: GPIO[42]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0
I (426) pp: pp rom version: e7ae62f
I (427) net80211: net80211 rom version: e7ae62f
I (429) wifi:wifi driver task: 3fca8304, prio:23, stack:6656, core=0
I (436) wifi:wifi firmware version: 91b9630
I (437) wifi:wifi certification version: v7.0
I (441) wifi:config NVS flash: enabled
I (445) wifi:config nano formating: disabled
I (449) wifi:Init data frame dynamic rx buffer num: 32
I (454) wifi:Init static rx mgmt buffer num: 5
I (458) wifi:Init management short buffer num: 32
I (462) wifi:Init dynamic tx buffer num: 32
I (466) wifi:Init static tx FG buffer num: 2
I (470) wifi:Init static rx buffer size: 1600
I (474) wifi:Init static rx buffer num: 10
I (478) wifi:Init dynamic rx buffer num: 32
I (482) wifi_init: rx ba win: 6
I (486) wifi_init: tcpip mbox: 32
I (490) wifi_init: udp mbox: 6
I (493) wifi_init: tcp mbox: 6
I (497) wifi_init: tcp tx win: 5760
I (501) wifi_init: tcp rx win: 5760
I (505) wifi_init: tcp mss: 1440
I (509) wifi_init: WiFi IRAM OP enabled
I (514) wifi_init: WiFi RX IRAM OP enabled
I (519) wifi:Set ps type: 0, coexist: 0

I (523) phy_init: phy_version 620,ec7ec30,Sep  5 2023,13:49:13
I (566) wifi:mode : sta (f4:12:fa:59:9e:ac)
I (566) wifi:enable tsf
I (568) espnow: esp-now Version: 2.4.0
I (569) ESPNOW: espnow [version: 1.0] init
I (570) espnow: mac: f4:12:fa:59:9e:ac, version: 2
I (575) espnow: Enable main task
I (579) espnow: main task entry
I (584) gpio: GPIO[5]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (592) gpio: GPIO[8]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (601) gpio: GPIO[9]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (610) gpio: GPIO[40]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (620) main_task: Returned from app_main()
I (3613) espnow_ctrl: bind, esp_log_timestamp: 3613, timestamp: 30584, rssi: -38, rssi: -55
I (3613) control_func: addr: 30:30:f9:5a:c9:28, initiator_type: 2, initiator_value: 1
I (3620) joystick_rc_receiver: bind, uuid: 30:30:f9:5a:c9:28, initiator_type: 513
I (8720) joystick_rc_receiver: About to turn left.
I (11920) joystick_rc_receiver: About to turn right.
I (12820) joystick_rc_receiver: About to turn right.
I (19164) joystick_rc_receiver: Forward.
I (19205) joystick_rc_receiver: Forward.
I (19228) joystick_rc_receiver: Forward.
I (19271) joystick_rc_receiver: Forward.
I (20735) joystick_rc_receiver: Backward.
I (20766) joystick_rc_receiver: Backward.
I (20808) joystick_rc_receiver: Backward.
I (20848) joystick_rc_receiver: Backward.
I (24427) joystick_rc_receiver: Turn left.
I (24457) joystick_rc_receiver: Turn left.
I (24492) joystick_rc_receiver: Turn left.
I (25448) joystick_rc_receiver: Turn right.
I (25477) joystick_rc_receiver: Turn right.
I (25558) joystick_rc_receiver: Turn right.
I (25593) joystick_rc_receiver: Turn right.
I (25619) joystick_rc_receiver: Turn right.
I (27664) joystick_rc_receiver: Braking...
I (27683) joystick_rc_receiver: Braking...
I (27713) joystick_rc_receiver: Braking...
I (27764) joystick_rc_receiver: Braking...
I (27779) joystick_rc_receiver: Braking...
I (27823) joystick_rc_receiver: Braking...
I (27846) joystick_rc_receiver: Braking...
I (27870) joystick_rc_receiver: Braking...
I (27901) joystick_rc_receiver: Braking...
```
