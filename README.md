# XIAO Webcam
An ESP-IDF project demonstrating using Seeed Studio's [XIAO ESP32S3 Sense](https://www.seeedstudio.com/XIAO-ESP32S3-Sense-p-5639.html) as a usb webcam.

![docs/xiao-esp32s3-sense.jpg](docs/xiao-esp32s3-sense.jpg?raw=true)

## Try it out

Got your board at hand? Download the latest [release](https://github.com/KamranAghlami/XIAO_Webcam/releases/latest) and flash online via [ESP Tool](https://espressif.github.io/esptool-js). Use the provided `offsets.json` file to specify which address to flash each bin file to.


## Flash via ESPTool

```bash
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 460800 write_flash \
  0x0 bootloader.bin \
  0x8000 partition-table.bin \
  0xe000 ota_data_initial.bin \
  0x10000 XIAO_Webcam.bin
```

or merge into one binary and then use the webtool:

```bash
esptool.py --chip esp32s3 merge_bin -o merged_firmware.bin \
  0x0 bootloader.bin \
  0x8000 partition-table.bin \
  0xe000 ota_data_initial.bin \
  0x10000 XIAO_Webcam.bin
```

then

```bash
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 460800 write_flash 0x0 merged_firmware.bin
```
