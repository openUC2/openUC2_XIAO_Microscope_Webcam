/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

// This file must be included AFTER usb_device_uvc.h
// to ensure uvc_frame_info_t is defined

#ifdef __cplusplus
extern "C" {
#endif

/* UVC Frame Configuration for XIAO ESP32S3 Webcam
 * This file defines the supported frame resolutions and formats for the UVC device.
 * Higher resolutions provide better image quality but may reduce frame rate.
 */

#ifdef CONFIG_CAMERA_MULTI_FRAMESIZE
// Multi-framesize configuration: supports multiple resolutions
// The host can select from these options in camera settings
#define UVC_FRAMES_COUNT 4

static const uvc_frame_info_t UVC_FRAMES_INFO[][UVC_FRAMES_COUNT] = {
    {
        // MJPEG Format with multiple resolution options
        {
            .width = 640,
            .height = 480,
            .rate = 15,          // VGA @ 15fps - good balance
            .intervalType = UVC_FRAME_INTERVAL_TYPE_DISCRETE,
            .interval = {
                .discrete = {
                    .numerator = 1,
                    .denominator = 15,
                }
            }
        },
        {
            .width = 800,
            .height = 600,
            .rate = 15,          // SVGA @ 15fps - higher quality
            .intervalType = UVC_FRAME_INTERVAL_TYPE_DISCRETE,
            .interval = {
                .discrete = {
                    .numerator = 1,
                    .denominator = 15,
                }
            }
        },
        {
            .width = 1280,
            .height = 720,
            .rate = 10,          // HD @ 10fps - HD quality
            .intervalType = UVC_FRAME_INTERVAL_TYPE_DISCRETE,
            .interval = {
                .discrete = {
                    .numerator = 1,
                    .denominator = 10,
                }
            }
        },
        {
            .width = 1920,
            .height = 1080,
            .rate = 5,           // Full HD @ 5fps - maximum quality
            .intervalType = UVC_FRAME_INTERVAL_TYPE_DISCRETE,
            .interval = {
                .discrete = {
                    .numerator = 1,
                    .denominator = 5,
                }
            }
        }
    }
};
#else
// Single framesize configuration: only one resolution available
// More stable for basic use cases
#define UVC_FRAMES_COUNT 1

static const uvc_frame_info_t UVC_FRAMES_INFO[][UVC_FRAMES_COUNT] = {
    {
        {
            .width = 800,
            .height = 600,
            .rate = 15,          // SVGA @ 15fps - default single mode
            .intervalType = UVC_FRAME_INTERVAL_TYPE_DISCRETE,
            .interval = {
                .discrete = {
                    .numerator = 1,
                    .denominator = 15,
                }
            }
        }
    }
};
#endif

#ifdef __cplusplus
}
#endif
