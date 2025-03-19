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



# Install IDF 

```
cd /Users/bene/Downloads/openUC2_XIAO_Microscope_Webcam/
git clone -b v5.3 https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh
. ./export.sh
conda deactivate
conda deactivate
cd /Users/bene/Downloads/openUC2_XIAO_Microscope_Webcam/esp-idf
source export.sh
cd /Users/bene/Downloads/openUC2_XIAO_Microscope_Webcam/
idf.py menuconfig

cd /Users/bene/Downloads/openUC2_XIAO_Microscope_Webcam
idf.py build IDF_TARGET=esp32s3 
idf.py build && idf.py flash
```