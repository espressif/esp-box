# ChatGPT Example

| Board             | Support Status |
| ----------------- | -------------- |
| ESP32-S3-BOX      | YES            |
| ESP32-S3-BOX-Lite | YES            |


In this example, we are utilizing the OpenAI API in conjunction with an ESP-BOX to create a voice-based chatbot. The ESP-BOX is a device or system that incorporates an ESP32-S3 microcontroller. The purpose of this implementation is to enable users to communicate with the chatbot using spoken language. The process involves capturing audio input from the user, sending it to the OpenAI API for processing, and receiving a response that is then converted into speech and played back to the user.

## How to use example
* ESP-IDF version [master](https://github.com/espressif/esp-idf)

```
git checkout master && git pull && git submodule update --init --recursive

```
* If you encounter any errors during the upgrade process, please make sure to use the ESP-IDF version [master](https://github.com/espressif/esp-idf) at commit ID `53ff7d43dbff642d831a937b066ea0735a6aca24`. This specific commit represents a stable state of the ESP-IDF repository and can help ensure a smoother transition or troubleshooting process.
```
git checkout 53ff7d43dbff642d831a937b066ea0735a6aca24 && git pull && git submodule update --init --recursive

```

### Hardware Required

* A ESP32-S3-BOX or ESP32-S3-BOX-Lite
* An USB-C cable for power supply and programming


## **Text to speech**
Due to the lack of native text-to-speech support in the [OpenAI](https://platform.openai.com/docs/api-reference) API, an external API is used to meet this requirement. This example utilizes the text-to-speech functionality offered by [TalkingGenie](https://www.talkinggenie.com/tts). Additional information can be found in this [blog post](https://czyt.tech/post/a-free-tts-api/?from_wecom=1).

### **Build and Flash**
There is another project called **factory_nvs** within the **ChatGPT_demo** project. It includes the code to store credentials in the NVS (Non-Volatile Storage) of the ESP-Box. On the other hand, **Chat_GPT Demo**consists of the demo code. Therefore, it is essential to build both projects.


**1. Clone the Github repository**

```bash
git clone https://github.com/espressif/esp-box

```

**2. Change the working directory**

```bash
cd examples/chatgpt_demo/factory_nvs

```

**3. Hardware Selection** 

To select the appropriate hardware ([ESP32-S3-BOX](https://github.com/espressif/esp-box/blob/master/docs/hardware_overview/esp32_s3_box/hardware_overview_for_box.md) or [ESP32-S3-BOX-Lite](https://github.com/espressif/esp-box/blob/master/docs/hardware_overview/esp32_s3_box_lite/hardware_overview_for_lite.md)) and access the HMI Board Config, use the following command.

Select the proper hardware  by using this command and look for **HMI Board Cofig**

```bash
idf.py menuconfig 

```

**4. Build the factory_nvs**

```bash
idf.py build

```

**5. Change the working directory**

```bash
cd examples/chatgpt_demo/

```
**6. Hardware Selection** 

To select the appropriate hardware ([ESP32-S3-BOX](https://github.com/espressif/esp-box/blob/master/docs/hardware_overview/esp32_s3_box/hardware_overview_for_box.md) or [ESP32-S3-BOX-Lite](https://github.com/espressif/esp-box/blob/master/docs/hardware_overview/esp32_s3_box_lite/hardware_overview_for_lite.md)) and access the HMI Board Config, use the following command.

```bash
idf.py menuconfig 

```

**7. Build the chatgpt_demo**

```bash
idf.py build

```

**8. Flash**

```bash
python -m esptool -p /dev/ttyACM0 --chip esp32s3 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0xd000 build/ota_data_initial.bin 0x10000 build/chatgpt_demo.bin 0x900000 build/storage.bin 0xb00000 build/srmodels/srmodels.bin 0x700000 factory_nvs/build/factory_nvs.bin

```

## Known compilation errors
When encountering compilation errors related to the "espressif__esp-sr" component, a common solution is to remove the ".component_hash" file located at "managed_components/espressif__esp-sr" and proceed with the rebuild. This step helps resolve the issue and allows the compilation process to continue smoothly.


## **Resources**
Follow the [blog posts](https://blog.espressif.com/) for brief description about the code.

1. [Unleashing the Power of OpenAI and ESP-BOX: A Guide to Fusing ChatGPT with Espressif SOCs](https://blog.espressif.com/unleashing-the-power-of-openai-and-esp-box-a-guide-to-fusing-chatgpt-with-espressif-socs-fba0b2d2c4f2)
2. [OpenAI Component | Accelerating the integration of OpenAI APIs in projects](https://blog.espressif.com/openai-component-accelerating-the-integration-of-openai-apis-in-projects-e5fa87998126)

### **Note**: 
Please note that, 
1. You require an **OpenAI API key** to proceed with the demo. 
2. To provide the WIFI credentials and the OpenAI secret key, Please follow the on display prompts to proceed.
3. Additionally, as a result of **OpenAI's restrictions**, this particular example cannot be supported within Mainland China.
4. ChatGPT is a large language model that is unable to distinguish between Chinese Jian ti (简体) and Fan ti (繁體) characters. Due to the constraints of LVGL (Light and Versatile Graphics Library), this project currently only supports Jian Ti (simplified Chinese characters).
