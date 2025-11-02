/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* UVC Frame Configuration for XIAO ESP32S3 Webcam
 * This file defines the supported frame resolutions and formats for the UVC device.
 * Higher resolutions provide better image quality but may reduce frame rate.
 * 
 * Note: The actual frame configuration array is defined in usb_webcam_main.c
 * to avoid header dependency issues with the UVC component types.
 */

#ifdef CONFIG_CAMERA_MULTI_FRAMESIZE
#define UVC_FRAMES_COUNT 4
#else
#define UVC_FRAMES_COUNT 1
#endif

#ifdef __cplusplus
}
#endif
