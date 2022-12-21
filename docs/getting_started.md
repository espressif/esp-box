* [中文版本](./getting_started_cn.md)

The guide is applicable to ESP32-S3-BOX Kits and ESP32-S3-BOX-Lite Kits with the latest version of firmware flashed. They are collectively referred to as BOX series of development boards in this guide.

# Getting Started

The BOX series of development boards integrated with ESP32-S3 SoCs provide a platform for users to develop the control system of smart home appliances, using the voice assistance + touch screen controller, sensor, infrared remote controller, and intelligent Wi-Fi gateway. The BOX series of development boards come with the pre-built firmware that supports voice wake-up and offline speech recognition both in Chinese and English. The ESP-BOX SDK features reconfigurable AI voice interaction, allowing you to customize commands to control home appliances. This guide briefly introduces what you can do with the latest version of firmware to help you get started. After you read through the guide, you may start developing an application by yourself. Now, let’s begin! 

**The BOX Kit contains:**

| ESP32-S3-BOX | ESP32-S3-BOX-Lite |
| :-----: | :---------------------: |
| A main unit that can work on its own | A main unit that can work on its own|
| An RGB LED module and Dupont wires for testing   | An RGB LED module and Dupont wires for testing|
| Dock, an accessory that serves as a stand for the main unit |  N/A|

**Required Hardware:**

Please find yourself a USB-C cable.

## Connect the RGB LED Module to Your Device

Please refer to the pin definition below, and connect the RGB LED module to the BOX using DuPont wires. The module has four male pins, R, G, B, and GND. Please connect them to the female ports G39, G40, G41, and GND on PMOD 1.

<div align="center">
<img src="./_static/_get_started_static/hardware_pmod.png">
</div>

## Power on Your Device
1. Power on your device using the USB-C cable.
<div align="center">
<img src="./_static/_get_started_static/usb_power.png" width="800px">
</div>

2. Once the device is powered on, the screen will play the Espressif logo boot animation.   

<div align="center">
<img src="./_static/_get_started_static/boot_animation.png" width="700px">
</div>

## Let's Play Around!

1. The first two pages of the quick guide introduce what the buttons do on your BOX series of development boards. Tap `Next` to go to the next page.

<table border="0" align="center">
<tr>
<td><b>ESP32-S3-BOX </b></td>
<td><b>ESP32-S3-BOX-Lite </b></td>
</tr>
<tr>
<td><img src="./_static/_get_started_static/0.png" width="300px"/></td>
<td><img src="./_static/_get_started_static/1.png" width="300px"/></td>
</tr>
 <tr>
<td><img src="./_static/_get_started_static/2.png" width="280px"/></td>
<td><img src="./_static/_get_started_static/2.png" width="280px"/></td>
</tr>
</table>

2. The last two pages of the quick guide introduces how to use AI voice control. Tap `OK Let’s Go` to enter the menu.

<table border="0" align="center">
<tr>
  <td><img src="./_static/_get_started_static/3_EN.png"></td>
  <td><img src="./_static/_get_started_static/3.1_EN.png"></td>
</tr>
</table>

3. There are five options in the menu: `Device Control`, `Network`, `Media Player`, `Help`, and `About Us`. You can navigate to different options by swiping left and right. For example, enter `Device Control` screen, tap `Light` to turn on or off the LED light on the module. Then you may go back to the menu, and enter `Media Player` screen to play music or adjust the volume.

<table border="0" align="center">
<tr>
<td><img src="./_static/_get_started_static/4.png"></td>
<td><img src="./_static/_get_started_static/4.1.png"></td>
</tr>
<tr>
<td><img src="./_static/_get_started_static/5.png"></td>
<td><img src="./_static/_get_started_static/5.1.png"></td>
</tr>
</table>

**Only ESP32-S3-BOX supports the following features:**

4. Press the mute button on the top of the device to disable voice wake-up and speech recognition. Press again to enable them.
   <div align="center">
   <img src="_static/_get_started_static/hardware_mute_button.jpg" width="350px">
   </div>

5. Tap the red circle below the screen to return to the last page.
   <div align="center">
   <img src="./_static/_get_started_static/hardware_home.png" width="350px">
   </div>

## Offline Voice Assistant

1. You may say "hi E. S. P." at any screen to wake up your device. When it wakes up, the screen will display the wake word you just used. If the wake word is not displayed, give it another shot. The screen below indicates your device is listening. 

<div align="center">
<img src="./_static/_get_started_static/17.png">
</div>

2. Utter a command within 6 seconds after the beep, such as "turn on the light". You will see the command shown on the screen and the LED light on the module turned on, and hear "OK". Around 6 seconds later you will exit the voice control screen if there is no more command.

<div align="center">
<img src="./_static/_get_started_static/21.png">
</div>

3. You can use voice commands to enjoy music. Please wake up the device first, then say "play music". The music player will open and start playing built-in music. You can also use voice commands to skip songs or pause music. There are two build-in songs.
   
   >**Tips:**
   
   >* If the LED light fails to turn on, check whether the module pins are inserted into the right ports.
   >* If the BOX recognizes no command within the specified time, you will see `Timeout` and exit the screen in about 1 second.
   
<div align="center">
<img src="_static/_get_started_static/19.png">
</div>

4. The default commands are: **turn on the light**, **turn off the light**, **turn red**, **turn green**, **turn blue**, **play music**, **next song**, **pause playing**.

## Continuous Speech Recognition 

More interestingly, the device supports continuous speech recognition after waking up. This feature makes voice interaction natural and smooth, and brings a human touch to interactive experience.

**How to use**

- Say "hi, E. S. P" to wake up the device, and you will hear a beep.
- Say your command. If the device recognizes the command, you will hear "OK", and then it will continue to recognize other commands.
- If no command is recognized, the device will wait. If there is not any command in 6 seconds, the device will automatically exit the voice control screen and you need to wake it up again.

**Attention**

- If the device fails to recognize your command for many times, please wait for timeout and wake it up again to use the feature.
- After you say the wake-up word, please do not move the device. Otherwise, the device will fail to recognize your command.
- We recommend voice commands of 3-5 words.
- Currently, the device cannot recognize commands when it plays prompts.

## Voice Command Customization

The BOX series of development boards are equipped with Espressif proprietary AI Speech Recognition System, which allows you to customize commands through our ESP BOX app. We will take the LED light on the module as an example, to show how to create your own voice commands. For algorithm details, please refer to [Technical Architecture](https://github.com/espressif/esp-box/blob/master/docs/technical_architecture.md).

**1. Connect to the ESP BOX mobile app**

1.1. Enter `Network`, and tap `To install APP` at the upper right corner. Scan the QR Code or search "ESP BOX" in App Store or Google Play to install the app.

   <div align="center">
   <img src="_static/_get_started_static/8.png">
   </div>
   <div align="center">
   <img src="_static/_get_started_static/Picture1.png" width="100 px">
   </div>
   
1.2. If you are new to this app, please register an account first.

1.3. Sign in with your ESP BOX account and turn on the Bluetooth on your phone. Tap `+` at the bottom of the screen, and scan the QR code on your device to set up the network.

<table border="0" align="center">
<tr>
  <td><img src="_static/_get_started_static/1.jpeg"width="300"/></td>
  <td><img src="_static/_get_started_static/2.jpeg"width="300"/></td>
  <td><img src="_static/_get_started_static/3.jpeg"width="300"/></td>
</tr>
</table>

1.4 After adding the device, you will see the following prompts:

<table border="0" align="center">
<tr>
  <td><img src="_static/_get_started_static/4.jpeg"width="300"/></td>
  <td><img src="_static/_get_started_static/5.jpeg"width="300"/></td>
</tr>
</table>

<div align="center">
<img src="_static/_get_started_static/7.png">
</div>

>**Tips:**
   
   >* Make sure you connect the device to 2.4 GHz Wi-Fi instead of 5 GHz, and enter the correct Wi-Fi password. If the Wi-Fi password is incorrect, the prompt "Wi-Fi Authentication failed" will pop up.
   >* Long press the `Boot` button (i.e. `Funtion` button) for 5 seconds to clear the network information and restore the device to factory settings. After the device is reset, if the QR code or Bluetooth is not working, please restart your device by pressing the `Reset` button.
   
   <div align="center">
   <img src="_static/_get_started_static/7.jpeg"width="300" />
   </div>

   **2. Customize Voice Commands**

2.1. Tap the ESP-BOX device icon and enter the below screen. You can turn on or off the light easily by toggling the button as shown in the picture. You may develop the Fan and Switch features by yourself.

<table border="0" align="center">
<tr>
  <td><img src="_static/_get_started_static/8.jpeg"width="300"/></td>
  <td><img src="_static/_get_started_static/9.jpeg"width="300"/></td>
</tr>
</table>

2.2 Tap `Light`, and the `Configure` tab shows the default pin information and commands. The pins for Red, Green, and Blue can be changed as needed.

<table border="0" align="center">
<tr>
  <td><img src="_static/_get_started_static/10 EN.jpeg"width="300"/></td>
  <td><img src="_static/_get_started_static/11.jpeg"width="300"/></td>
</tr>
</table>

2.3 In the `Configure` tab, you may also customize commands to turn on or off the light and change its color. For example, you can set "good morning" as the command to turn on the light. Click `Save` to return to the previous screen. Then click `Save` again as shown below.

<table border="0" align="center">
<tr>
  <td><img src="_static/_get_started_static/13 EN.jpeg"width="300"/></td>
  <td><img src="_static/_get_started_static/13.1 EN.jpeg"width="300"/></td>
</tr>
</table>

2.4 In the `Control` tab, you may adjust the color, brightness, and saturation of the light.

   <div align="center">
   <img src="_static/_get_started_static/12.jpeg"width="300" />
   </div>

2.5 Now, you can try out your new command! First, say "hi E. S. P." to wake up your device. Then say "good morning" within 6 seconds to turn on the light. The new command will show on the screen with the module light turned on.

   <div align="center">
   <img src="_static/_get_started_static/15.png">
   </div>

>**To ensure the commands work well, please note:**
>
>* Length of commands: A command should consists of 2-8 words. When creating a series of commands, please try to keep them at similar lengths.
>* Avoid repeating: Please do not include shorter commands in longer commands, or shorter commands will not be recognized. For example, if you create both "turn on" and "turn on the light" commands, "turn on" will not be recognized.

   **3. switch CN and EN voice model：**

3.1. Click on your ESP-BOX `About Us` device icon to enter the `About Us` details page. In this page, press `Boot` button, the device will enter `factory Mode` language setting page, and then you can select the voice wake-up model by yourself.

   <div align="center">
   <img src="_static/_get_started_static/language_select.png"width="300" />
   </div>

## FCC Regulations:

This device complies with part 15 of the FCC Rules. Operation is subject to the following two conditions: (1) This device may not cause harmful interference, and (2) this device must accept any interference received, including interference that may cause undesired operation. This device has been tested and found to comply with the limits for a Class B digital device, pursuant to Part 15 of the FCC Rules. These limits are designed to provide reasonable protection against harmful interference in a residential installation. This equipment generates, uses and can radiated radio frequency energy and, if not installed and used in accordance with the instructions, may cause harmful interference to radio communications. However, there is no guarantee that interference will not occur in a particular installation. If this equipment does cause harmful interference to radio or television reception, which can be determined by turning the equipment off and on, the user is encouraged to try to correct the interference by one or more of the following measures:

- Reorient or relocate the receiving antenna.
- Increase the separation between the equipment and receiver.
- Connect the equipment into an outlet on a circuit different from that to which the receiver is connected.
- Consult the dealer or an experienced radio/TV technician for help.

**FCC Note** Caution: Changes or modifications not expressly approved by the party responsible for compliance could void the user‘s authority to operate the equipment.

## RF Exposure Information

This device meets the government’s requirements for exposure to radio waves.

This device is designed and manufactured not to exceed the emission limits for exposure to radio frequency (RF) energy set by the Federal Communications Commission of the U.S. Government.

This device complies with FCC radiation exposure limits set forth for an uncontrolled environment. In order to avoid the possibility of exceeding the FCC radio frequency exposure limits, human proximity to the antenna shall not be less than 20 cm during normal operation.

The guide only gives you a brief idea of how to use the latest firmware on your BOX series of development boards. Now, you may start writing programs, and embark on your IoT journey!
