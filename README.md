# Espressif HMI Development Framework

* [中文版](./docs/README_cn.md)

Espressif Systems HMI Development Framework (ESP-HMI) is the official HMI development framework for the [ESP32](https://espressif.com/en/products/hardware/esp32/overview), [ESP32-S2](https://www.espressif.com/en/products/socs/esp32-s2), [ESP32-S3](https://www.espressif.com/en/products/socs/esp32-s3) and [ESP32-C3](https://www.espressif.com/en/products/socs/esp32-c3) SoCs.

## Overview

ESP-HMI supports development of HMI applications for the Espressif Systems SoCs in the most comprehensive way. With ESP-HMI, you can easily add features, develop HMI applications from simple to complex:

- Light up LCD and change pixels of it.
- Display image on LCD from Flash RO-data, SPIFFS, SD card, host via JTAG or network.
- Using TTF/OTF fonts to display CJK fonts or change it's size as you wish.
- Develop fancy GUI demo.

## Framework

ESP-HMI contains

## Development with ESP-HMI

### Setting Up ESP-IDF

See https://idf.espressif.com/ for links to detailed instructions on how to set up the ESP-IDF depending on chip you use.

**Note:** Each SoC series and each ESP-IDF release has its own documentation. Please see Section [Versions](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/versions.html) on how to find documentation and how to checkout specific release of ESP-IDF.

>  **Non-GitHub forks**
>
> ESP-IDF uses relative locations as its submodules URLs ([.gitmodules](https://github.com/espressif/esp-idf/blob/master/.gitmodules)). So they link to GitHub. If ESP-IDF is forked to a Git repository which is not on GitHub, you will need to run the script [tools/set-submodules-to-github.sh](https://github.com/espressif/esp-idf/blob/master/tools/set-submodules-to-github.sh) after git clone. The script sets absolute URLs for all submodules, allowing `git submodule update --init --recursive` to complete. If cloning ESP-IDF from GitHub, this step is not needed.

### Get ESP-HMI

### Update Sub-modules

It is not necessary for you to update sub-modules if you used recursive clone.



You can run `git submodule update --init --recursive` to checkout all sub-modules.

## Examples

- Get-Started
  - Basic LCD operation and input 
  - Image display with raw LCD APIs
  - An camera receiver
- LVGL Examples
  - Demos provided by LVGL
  - Project template
  - Display image using official image display repos
  - Vector font render with FreeType
  - A GUI provision demo
- Audio Examples
  - A simple music player
  - A simple recorder

