# **ChatGPT Example**
In this example, we are utilizing the OpenAI API in conjunction with an ESP-Box to create a voice-based chatbot. The ESP-Box is a device or system that incorporates an ESP32-S3 microcontroller. The purpose of this implementation is to enable users to communicate with the chatbot using spoken language. The process involves capturing audio input from the user, sending it to the OpenAI API for processing, and receiving a response that is then converted into speech and played back to the user.

## **Environment**
1. ESP-IDF version [master](https://github.com/espressif/esp-idf)

```
git checkout master && git pull && git submodule update --init --recursive

```
* If you encounter any errors during the upgrade process, please make sure to use the ESP-IDF version [master](https://github.com/espressif/esp-idf) at commit ID `df9310ada26123d8d478bcfa203f874d8c21d654`. This specific commit represents a stable state of the ESP-IDF repository and can help ensure a smoother transition or troubleshooting process.
```
git checkout df9310ada26123d8d478bcfa203f874d8c21d654 && git pull && git submodule update --init --recursive

```

## **Text to speech**
Due to the lack of native text-to-speech support in the [OpenAI](https://platform.openai.com/docs/api-reference) API, an external API is used to meet this requirement. This example utilizes the text-to-speech functionality offered by [TalkingGenie](https://www.talkinggenie.com/tts). Additional information can be found in this [blog post](https://czyt.tech/post/a-free-tts-api/?from_wecom=1).

## **Reproducing the demo**

### 1. Clone the Github repository

```bash
git clone https://github.com/espressif/esp-box

```

### 2. Change the working directory to model_deployment

```bash
cd examples/chatgpt_demo/

```

### 3. Set up OpenAI Key, WiFi SSID and Password 

```
idf.py menuconfig

```
### 4. Build the project

```bash
idf.py build flash monitor

```
In case found error during the building process [follow the official IDF  guide for more details](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#build-your-first-project).

> **Note :**
> Please note that you require an OpenAI API key to proceed with the demo. To enter the WIFI credentials and OpenAI secret key, access the menuconfig using "idf.py menuconfig"
