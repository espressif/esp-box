# JoyStick-RC-Receiver Example

[[English]](README.md)

该示例为 [ESP-JoyStick](../joystick_controller) 项目的航模接收机功能模块，使用该示例程序可实现对 RC 遥控车的控制，控制效果如下：

* 通过 ESP-NOW 与 ESP-JoyStick 配对连接成功后，RGB 指示灯亮绿灯；未配对连接时，RGB指示灯红灯常亮。
* 左侧摇杆向上/向下推，小车前进/后退。
* 左侧摇杆向下推，小车后退，黄色倒车灯亮起。
* 右侧摇杆向左/向右推，小车左转/右转。
* 按下按键 A ，小车刹车，红色刹车灯亮起。
* 按下按键 LB/RB, 左/右转向灯闪烁，再次按下 LB/RB 或右侧摇杆向左/向右推关闭左/右转向灯。

<div align="center">
<img src="https://dl.espressif.com/ae/esp-box/control_rc_car.gif/control_rc_car.gif" width="60%">
<p>ESP-JoyStick 控制 RC 遥控车</p>
</div>

## 硬件说明

该示例程序基于 [ESP32-S3-USB-Bridge](https://docs.espressif.com/projects/espressif-esp-dev-kits/zh_CN/latest/esp32s3/esp32-s3-usb-bridge/user_guide.html) 开发板（购买链接：[淘宝](https://item.taobao.com/item.htm?ft=t&id=753321694177)或[速卖通](https://www.aliexpress.us/item/3256806114330511.html?gatewayAdapt=glo2usa4itemAdapt)）进行开发，具体引脚分配如下：
| 引脚             | 功能说明 |
| ----------------- | -------------- |
| GPIO42   | ESP-NOW 连接状态指示灯引脚  |
| GPIO2    | 小车前进 PWM 控制引脚 |
| GPIO3    | 小车后退 PWM 控制引脚 |
| GPIO4    | 小车转向舵机 PWM 信号控制引脚 |
| GPIO8    | 左转向灯控制引脚，高电平亮低电平灭 |
| GPIO9    | 右转向灯控制引脚，高电平亮低电平灭 |
| GPIO5    | 刹车灯控制引脚，高电平亮低电平灭 |
| GPIO40   | 倒车灯控制引脚，高电平亮低电平灭 |

## 如何使用本示例

### 编译和烧录

运行 `idf.py flash monitor` 命令进行编译、烧录和查看日志。

### 示例输出

运行此示例，您将看到如下输出日志:

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
