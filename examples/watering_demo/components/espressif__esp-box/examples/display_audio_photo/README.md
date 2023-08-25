# ESP-BOX Display Audio Photo Example

This example demonstrates usage of ESP-BOX Board Support Package. This is a single purpose example, which is focused on display + touch applications: you can see list of files saved on SPIFFS. You can open certain file types like JPG images, WAV music files and TXT text files. 

Example files are downloaded into ESP-BOX from [spiffs_content](/spiffs_content) folder.

## How to use the example

### Hardware Required

* ESP-BOX
* USB-C Cable

### Compile and flash

```
idf.py -p COMx flash monitor
```

### Example outputs

After initialization:
```
...
I (745) gpio: GPIO[4]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (745) gpio: GPIO[48]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (875) ESP-BOX: Starting LVGL task
I (875) ESP-BOX: Setting LCD backlight: 50%
I (1565) ES8311: ES8311 in Slave mode and I2S format
I (1575) I2S: DMA Malloc info, datalen=blocksize=2048, dma_desc_num=3
I (1575) I2S: DMA Malloc info, datalen=blocksize=2048, dma_desc_num=3
I (1575) I2S: I2S1, MCLK output by GPIO2
I (1575) gpio: GPIO[46]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (1585) ESP-BOX: Example initialization done.
```

When text file selected:
```
I (142855) DISP: Clicked: Readme.txt
```

When JPG file selected:
```
I (81275) DISP: Clicked: Death Star.jpg
I (81275) DISP: Decoding JPEG image...
```

When music file selected:
```
I (184605) DISP: Clicked: imperial_march.wav
I (191135) DISP: Number of channels: 1
I (191135) DISP: Bits per sample: 16
I (191135) DISP: Sample rate: 22050
I (191135) DISP: Data size: 1763806
```
