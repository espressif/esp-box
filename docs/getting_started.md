* [中文版本](./getting_started_cn.md)

# Getting Started

The ESP32-S3-BOX gives you a simple idea of controlling the home appliances using Voice Assistance + touch screen controller, sensor, infrared controller, and intelligent Wi-Fi gateway via the ESP32-S3-BOX. With the abundant SDKs and solutions provided by Espressif, you will be able to develop a wide variety of AIoT applications such as online and offline voice assistants, voice-enabled devices, HMI touch-screen devices, control panels, multi-protocol gateways, with great ease. 

The most impressive part of the ESP32-S3-BOX is the AI voice interaction that allows you to change the command phrases to control the smart home appliances. The content of this material is only to give you a brief concept of how the ESP32-S3-BOX works, a basic idea for you to know where to begin. Once you go through the guide below, you may start developing the application yourself. So, let’s begin the journey! 

<div align="center">
<img src="_static/esp_s3_box_outline.png">
</div>

**What is in the ESP32-S3-BOX Kit:**

1. ESP32-S3-BOX main unit
2. Dock (Stand for ESP32-S3-BOX)
3. RGB LED Module & Dupont wires for testing

**What do you need:**

1. Type-C Cable (not included in the BOX Kit)


## Connect the RGB LED Module to the BOX

1. Refer to the pin definition below, insert the RGB LED Module to the BOX.
2. The RGB LED Module has four male pins: R, G, B, GND connect to the G39, G40, G41, GND female port of the PMOD 1.
3. Only one unit of RGB Module is provided in the kit.

<div align="center">
<img src="./_static/hardware_pmod.png">
</div>

## Power on the ESP32-S3-BOX

1. Use a Type-C cable (not included in the ESP32-S3-BOX kit) to power on the ESP32-S3-BOX.

<div align="center">
<img src="_static/plug_power.png">
</div>

2. Once power on, the interactive Espressif logo will display on the screen.

<div align="center">
<img src="./_static/boot_animation.png">
</div>

## Let’s play around with the ESP32-S3-BOX!

1. Initially, the “Steps for usage” pop-up window gives you a hint of how to use the AI Voice Assistance. 

   <div align="center">
   <img src="./_static/page_use_intr.png">
   </div>

2. Follow the “Steps for usage” or press the “OK Let’s Go” to skip this and return to the Home Screen.

   <div align="center">
   <img src="./_static/page_home.png">
   </div>

3. Similarly, you may press the “Red Circle” on the device and return to the Home Screen.

   <div align="center">
   <img src="./_static/hardware_home.png">
   </div>

4. Now, say the wake word “Hi ESP” to activate your ESP32-S3-BOX. If the board wakes up, its screen below will display the wake word you just spoke. If the wake word is not displayed, try the wake word more times.  The animation below gives you a hint that your device is listening.

   <div align="center">
   <img src="./_static/page_hi.png">
   </div>

5. Please voice out one command phrase within 6 seconds each time and say your command such as “**Turn on the light**”, the command phrase will display on the screen, and the RGB light of the LED Module will be turned on. Around 1s, the voice user interface jumps back to the Home Screen.

   <div align="center">
   <img src="_static/page_turn_on_the_light.png">
   </div>

6. The default command words are: "**Turn on the light**", "**Turn off the light**", "**Turn Red**", "**Turn Green**", "**Turn Blue**", "**Turn White**".

   <div align="center">
   <img src="_static/page_timeout.png">
   </div>

   >**Tips:**
   >* Check the connection of the RGB Module if the LED is not turned on. The pin of the RGB Module may be inserted into the wrong port of the BOX.
   >* If received no command within the specified time, the voice user interface will show “Timeout” and jump back to the Home Screen.

7. Press the Mute button on the top of the ESP32-S3-BOX to disable the voice wake-up and speech recognition function. Press again to restore the function.

   <div align="center">
   <img src="_static/hardware_mute_button.jpg">
   </div>

## Voice Assistance Control and Customization

The most impressive part of this ESP32-S3-BOX is equipped with the Espressif proprietary AI Voice Recognition system. Meaning, you could customize the command phrases used to give the instructions. For the details of how this works, please refer to our Developer Guide.

**Let’s begin to learn about the Voice Command Customization:**

**1. First, connect to the ESP32-S3-BOX Wi-Fi simply using your phone**

1.1. Click on the “Wi-Fi” logo from the Home Screen that appears at the upper left corner.

   <div align="center">
   <img src="_static/page_wifi_lable.png">
   </div>

1.2. You will see a QR Code (as below) once you press the Wi-Fi logo.

   <div align="center">
   <img src="_static/page_scan_qrcode.png">
   </div>

1.3. Simply turn on the QR code scanner APP on your phone or your phone’s Camera to do network provisioning. For iPhone users, you may turn on your Camera.

   <div align="center">
   <img src="_static/phone_scan_qrcode.png">
   </div>

**2. Visit Web Control Interface “192.168.4.1”**

Scan another QR code again, and it will redirect you to the web control interface. Alternatively, you can turn on the mobile phone browser, enter the IP address “192.168.4.1”.

   <div align="center">
   <img src="_static/page_scan_qrcode2.png">
   </div>

   <div align="center">
   <img src="_static/phone_scan_qrcode2.png">
   </div>

**3. Voice Command Customization**

   1. You can simply turn on & turn off the light from this user interface；

   <div align="center">
   <img src="_static/web_config_light.JPG">
   </div>

   2. Press on the “light” icon and navigate to the Voice Command Device Control page to check the current default pin information and command word；

   <div align="center">
   <img src="_static/web_config_light_cmd.JPG">
   </div>

   3. The "Control” allows you to make the necessary changes to the light, including color, brightness and saturation。

   <div align="center">
   <img src="_static/web_config_light_ctrl.JPG">
   </div>

   4. The “Voice" Section allows you to modify the preference command phrases to control the light **“ON”**, **“OFF”** and **“Color”**. For example, you can custom the **“Good morning”** for action **“ON”**, click save, the page return back to the previous interface, and click save again；

   <div align="center">
   <img src="_static/web_config_light_cmd_cg.JPG">
   </div>

   5. Now, you can try out your new command! First, say “Hi ESP” to wake-up your device. Then say **“Good morning”** to turn on your light within 6 seconds. The new command phrase will display on the screen and the LED module will be turned on as below.。

   <div align="center">
   <img src="_static/page_good_morning.png">
   </div>

## ESP32-S3-BOX Graphical User Interface

1. Press on the “function” logo from the Home Screen that appears at the upper right corner and navigate to the User Interface.

   <div align="center">
   <img src="./_static/page_panel_lable.png">
   </div>

2. Press on the “Light” icon will allow you to turn on and turn off the light.
3. Long press the “Light” icon to navigate to the Light Tuning Interface, which allows changing the color display. Long press the Light Tuning Circle again to adjust the light saturation and brightness, respectively.
4. The built-in function above is pre-programmed to give you a brief idea of how this ESP32-S3-BOX can be used in your project. 

   <div align="center">
   <img src="_static/page_panel_color.png">
   </div>

5. The other three icons (Media, Fan, Security) are added for illustration purposes, and you may start writing your exciting application program! Begin your IoT Journey!

   <div align="center">
   <img src="_static/page_panel_control.png">
   </div>

## ESP-BOX BBS

Visit the BBS [esp32.com](https://esp32.com/viewforum.php?f=44) to question us or contact us at sales@espressif.com, and we will reply to you as soon as possible.
