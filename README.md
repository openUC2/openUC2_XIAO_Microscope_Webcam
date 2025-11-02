# XIAO Webcam
An ESP-IDF project demonstrating using Seeed Studio's [XIAO ESP32S3 Sense](https://www.seeedstudio.com/XIAO-ESP32S3-Sense-p-5639.html) as a usb webcam.

![docs/xiao-esp32s3-sense.jpg](docs/xiao-esp32s3-sense.jpg?raw=true)

## Features

### 🎥 High-Resolution Support
- Multiple resolution options: VGA (640x480), SVGA (800x600), HD (1280x720), Full HD (1920x1080)
- Configurable via `CONFIG_CAMERA_MULTI_FRAMESIZE` in menuconfig
- Automatic JPEG quality adjustment per resolution for optimal performance

### 🔧 Easy Flashing
- **Boot Delay Feature**: 5-second delay before UVC device initialization
- Flash firmware without manually pressing boot button during this window
- Configurable via `CONFIG_UVC_BOOT_DELAY_MS` (default: 5000ms)
- LED blinks during boot delay to indicate waiting period

### 📷 Camera Settings
- Pre-configured sensor settings for optimal image quality
- Auto-exposure, auto-gain, and auto-white balance enabled by default
- Brightness, contrast, saturation, and other controls properly initialized
- Settings optimized for microscopy and imaging applications

### 🌐 OTA Updates
- Wi-Fi Access Point mode (SSID: "XIAO_AP", Password: "12345678")
- Web-based firmware update interface at http://192.168.4.1/
- Upload new firmware without physical access to the device

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

## Configuration

### Resolution and Frame Rate

The firmware supports multiple camera resolutions. You can configure this in `menuconfig`:

```bash
idf.py menuconfig
# Navigate to: USB WebCam config -> Enable multiple frame sizes
```

Available resolutions when multi-framesize is enabled:
- 640x480 @ 15fps (VGA) - Good balance of quality and performance
- 800x600 @ 15fps (SVGA) - Higher quality, recommended default
- 1280x720 @ 10fps (HD) - High definition quality
- 1920x1080 @ 5fps (Full HD) - Maximum quality for microscopy

When multi-framesize is disabled, only 800x600 @ 15fps is available (more stable).

### Boot Delay Configuration

To change the boot delay before UVC initialization:

```bash
idf.py menuconfig
# Navigate to: USB WebCam config -> UVC boot delay (milliseconds)
# Default: 5000ms (5 seconds)
# Range: 0-30000ms
# Set to 0 to disable boot delay
```

During the boot delay:
- The LED will blink to indicate the device is in the waiting period
- You can flash new firmware without pressing the boot button
- After the delay, UVC device starts normally

### Camera XCLK Frequency

Default XCLK frequency is 20MHz. You can adjust this:

```bash
idf.py menuconfig
# Navigate to: USB WebCam config -> XCLK frequency
# Default: 20000000 (20MHz)
# Range: 1000000-40000000
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