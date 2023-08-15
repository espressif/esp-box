# ESP32-S3-BOX Matter Switch

| Board             | Support Status |
| ----------------- | -------------- |
| ESP32-S3-BOX      | YES            |
| ESP32-S3-BOX-Lite | YES            |

This example creates a 3 channel matter On/Off Switch device on esp32s3 box platform.

It creates the On/Off client and other on-off devices can be bound to the switch and then controlled from the switch.
## 1. Esp-idf And Esp-matter Commit ID

esp-idf: master branch 67552c31dac8cd94fb0d63192a538f4f984c5b6e

esp-matter: main branch de3fc77b03c2bd0e59bca623290399ae5302a0f6
## 2. Simplified Environment Setup
### 2.1 Setup esp-matter environment

See the [docs](https://docs.espressif.com/projects/esp-matter/en/latest/esp32/developing.html#developing-with-the-sdk) for more information about esp-matter building and flashing the firmware.

### 2.2 Matter-switch example build and setup

After setup the esp-matter environment, get into the matter-switch project folder, then
execute below commands:

```
idf.py set-target esp32s3 build

cd build && esptool.py --chip esp32s3 merge_bin `cat flash_args` -o demo_box_s3_box_switch.bin

esptool.py -p /dev/ttyACM0 write_flash 0 demo_box_s3_box_switch.bin

cd ..

idf.py -p /dev/ttyACM0 monitor
```
After flash the firmware, then use the chip-tool to pairing the matter network, you can use the default command:

```
./chip-tool pairing ble-wifi 0x4 wifi-name wifi-password 20202021 3840
```

After pairing successfully, the s3 box will start displaying splash screen.

<img src="docs/_static/_get_started_static/boot_animation.png" width="200px" />


## 2.3 Open commissioning window and pairing using Homekit

After pairing using chip-tool, you can send below command to open commissioning window:

```
./chip-tool pairing open-commissioning-window 0x4 1 180 1000 3840
```

```
[1689931442.902596][2216299:2216301] CHIP:CTL: Successfully opened pairing window on the device
[1689931442.902603][2216299:2216301] CHIP:CTL: Manual pairing code: [36478701899]
[1689931442.902607][2216299:2216301] CHIP:CTL: SetupQRCode: [MT:MH5B4BFN00KCTW7I200]
[1689931442.902623][2216299:2216301] CHIP:DMG: ICR moving to [AwaitingDe]
```
Then we can use the ***SetupQRCode: [MT:MH5B4BFN00KCTW7I200]*** or ***Manual pairing code: [36478701899]*** to paring to Homekit, as below show:

<img src="docs/_static/matter/apple1.png" width="200px" />
<img src="docs/_static/matter/apple2.png" width="200px" />
<img src="docs/_static/matter/apple3.png" width="200px" />
<img src="docs/_static/matter/apple4.png" width="200px" />
<img src="docs/_static/matter/apple5.png" width="200px" />
<img src="docs/_static/matter/apple6.png" width="200px" />

## 3. Post Commissioning Setup

After Commissioning, we can bind threee matter on-off devices to the box switch and control them via touch screen and speech recognition.
### 3.1 Bind on-off devices to switch

Using the chip-tool, commission 4 devices, the box matter-switch and 3 on-off devices, such as onoff light, fan, onoff plug.
If you are having trouble, try commissioning them one at a time (by powering off the other device) as
the default discriminator and passcode are same for all of them.
Then use the below commands to bind the onoff devices to the switch.

For the commands below:

-   Node Id of switch used during commissioning is 4
-   Node Id of onoff light used during commissioning is 1
-   Node Id of onoff fan used during commissioning is 2
-   Node Id of onoff plug used during commissioning is 3
-   Cluster Id for OnOff cluster is 6
-   Binding cluster is currently present on endpoint 1, 2 and 3 on the switch

Update the 3 onoff devices' acl attribute to add the entry of remote device
(switch) in the access control list:
```
./chip-tool accesscontrol write acl '[{"fabricIndex": 1, "privilege": 5, "authMode": 2, "subjects": [ 112233, 4 ], "targets": null}]' 1 0x0
./chip-tool accesscontrol write acl '[{"fabricIndex": 1, "privilege": 5, "authMode": 2, "subjects": [ 112233, 4 ], "targets": null}]' 2 0x0
./chip-tool accesscontrol write acl '[{"fabricIndex": 1, "privilege": 5, "authMode": 2, "subjects": [ 112233, 4 ], "targets": null}]' 3 0x0
```

Update the switch's binding attribute to add the entry of remote device
(light) in the binding table:
```
./chip-tool binding write binding '[{"fabricIndex": 1, "node":1, "endpoint":1, "cluster":6}]' 4 0x1
```

(fan) in the binding table:
```
./chip-tool binding write binding '[{"fabricIndex": 1, "node":2, "endpoint":1, "cluster":6}]' 4 0x2
```

(plug) in the binding table:
```
./chip-tool binding write binding '[{"fabricIndex": 1, "node":3, "endpoint":1, "cluster":6}]' 4 0x3
```

### 3.2 Control 3 onoff devices

After binding successfully, we can control the three onoff devices via touch screen and speech recognition.
#### Touch screen control

Enter the device control interface, click the *Light*, *Fan*, *Switch* buttons respectively to control the onoff light, fan and onoff plug.

<img src="docs/_static/_get_started_static/4.png" width="200px" />
<img src="docs/_static/_get_started_static/4.1.png" width="200px" />

#### Speech recognition control

Current matter-switch example only support English mode, you can say "Hi ESP" to wake up the switch, then say

```
Turn on the light
Turn off the light
Turn on the fan
Turn off the fan
Turn on the switch
Turn off the switch
```

to control the three onoff devices.

<img src="docs/_static/_get_started_static/17.png" width="200px" />
<img src="docs/_static/_get_started_static/21.png" width="200px" />

## 4 Factory Reset Matter Switch

Long press *Boot* button for more than 5 seconds, then release, the matter switch will trigger factoryreset, all the matter related information will be factoryreset.
## A1 Appendix FAQs

### A1.1 Binding Failed

My light is not getting bound to my switch:

-   Make sure the light's acl is updated. You can read it again to make
    sure it is correct: `accesscontrol read acl 0x5164 0x0`.
-   If you are still facing issues, reproduce the issue on the default
    example for the device and then raise an [issue](https://github.com/espressif/esp-matter/issues).
    Make sure to share these:
    -   The complete device logs for both the devices taken over UART.
    -   The complete chip-tool logs.
    -   The esp-matter and esp-idf branch you are using.

