# ESP32-S3-Cube Factory Demo

This is the factory demo for ESP32-S3-Cube development board. 

## How to Use

### Hardware Required

* A ESP32-S3-Cube
* An USB cable for power supply and programming
* A 5mm infrared LED (e.g. IR333C) used to transmit encoded IR signals 

example connection:

|        | ESP32-S3-Cube |
| :----: | :-----------: |
| IR333C |     IO 42     |

### Build and Flash

Run `idf.py flash -p /dev/ttyACM0 monitor` to build, flash and monitor the project.

(To exit the serial monitor, type `Ctrl-]`.)

## Example Output

Run this example, you will see the following output log:

```
I (0) cpu_start: Starting scheduler on APP CPU.
I (568) spiram: Reserving pool of 32K of internal memory for DMA/internal allocations
I (568) gpio: GPIO[1]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:3 
I (568) gpio: GPIO[4]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (568) gpio: GPIO[48]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (688) gpio: GPIO[45]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (688) bsp_tp: Detected touch panel at 0x24. Vendor : Parade Tech
```
