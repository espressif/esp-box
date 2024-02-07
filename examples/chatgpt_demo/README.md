# ChatGPT Example

| Board             | Support Status |
| ----------------- | -------------- |
| ESP32-S3-BOX      | YES            |
| ESP32-S3-BOX-Lite | YES            |
| ESP32-S3-BOX-3    | YES            |


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

### **Hardware Required**

* A ESP32-S3-BOX，ESP32-S3-BOX-Lite or ESP32-S3-BOX-3
* A USB-C cable for power supply and programming


### **Build and Flash**
There is another project named **factory_nvs** within the **ChatGPT_demo** project. It includes the code to store credentials in the NVS (Non-Volatile Storage) of the ESP-Box. On the other hand, **Chat_GPT Demo** consists of the demo code. Therefore, it is essential to build both projects.


**1. Clone the Github repository**

```bash
git clone https://github.com/espressif/esp-box

```

**2. Change the working directory**

```bash
cd examples/chatgpt_demo/factory_nvs

```

**3. Hardware Selection**

To select the appropriate hardware ([ESP32-S3-BOX](https://github.com/espressif/esp-box/blob/master/docs/hardware_overview/esp32_s3_box/hardware_overview_for_box.md), [ESP32-S3-BOX-Lite](https://github.com/espressif/esp-box/blob/master/docs/hardware_overview/esp32_s3_box_lite/hardware_overview_for_lite.md) or [ESP32-S3-BOX-3](https://github.com/espressif/esp-box/blob/master/docs/hardware_overview/esp32_s3_box_3/hardware_overview_for_box_3.md)) and access the **HMI Board Cofig**, use the following command.


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

To select the appropriate hardware ([ESP32-S3-BOX](https://github.com/espressif/esp-box/blob/master/docs/hardware_overview/esp32_s3_box/hardware_overview_for_box.md), [ESP32-S3-BOX-Lite](https://github.com/espressif/esp-box/blob/master/docs/hardware_overview/esp32_s3_box_lite/hardware_overview_for_lite.md) or [ESP32-S3-BOX-3](https://github.com/espressif/esp-box/blob/master/docs/hardware_overview/esp32_s3_box_3/hardware_overview_for_box_3.md)) and access the **HMI Board Cofig**, use the following command.

```bash
idf.py menuconfig

```

**7. Build the chatgpt_demo**

```bash
idf.py build

```

**8. Flash**

```bash
idf.py -p PORT flash

```

## Known Issues
1. When encountering compilation errors related to the `espressif__esp-sr` component, a common solution is to remove the `.component_hash` file located at `managed_components/espressif__esp-sr` and proceed with the rebuild. This step helps resolve the issue and allows the compilation process to continue smoothly.
2. If you encounter an error related to **API Key is not valid**, please verify that you have entered your key correctly. Additionally, ensure that you have a sufficient number of valid tokens available to access the OpenAI server. You can login [OpenAI website](https://openai.com/) to confirm your token  [Usage status](https://platform.openai.com/account/usage).
3. The demo is initially configured with English as the default chatting language. If you wish to use Chinese, please make the following adjustment in the code: `audioTranscription->setLanguage(audioTranscription, "zh")`. However, please note that we cannot guarantee that ChatGPT's Chinese conversion and responses will achieve highly accurate results. Please feel free to modify and test this translation to suit your needs.


## **Resources**
Follow the [blog posts](https://blog.espressif.com/), [demos and tutorials](https://www.youtube.com/@EspressifSystems) for more updates.

1. BLOG: [Unleashing the Power of OpenAI and ESP-BOX: A Guide to Fusing ChatGPT with Espressif SOCs](https://blog.espressif.com/unleashing-the-power-of-openai-and-esp-box-a-guide-to-fusing-chatgpt-with-espressif-socs-fba0b2d2c4f2)
2. BLOG: [OpenAI Component | Accelerating the integration of OpenAI APIs in projects](https://blog.espressif.com/openai-component-accelerating-the-integration-of-openai-apis-in-projects-e5fa87998126)
3. Tutorial: [ESP Tutorial: Unleashing the Power of ESP32 S3 BOX 3 with OpenAI](https://www.youtube.com/watch?v=Y97vdw7y3S4&t=2s)

## **Note**
Please note that,
1. To proceed with the demo, you need an **OpenAI API key**, and you must possess valid tokens to access the OpenAI server.
2. To provide the WIFI credentials and the OpenAI secret key, please follow the on display prompts to proceed.
3. Additionally, as a result of **OpenAI's restrictions**, this particular example cannot be supported within Mainland China.
4. ChatGPT is a large language model that is unable to distinguish between Chinese Jian ti (简体) and Fan ti (繁體) characters. Due to the constraints of LVGL (Light and Versatile Graphics Library), this project currently only supports Jian Ti (simplified Chinese characters).
