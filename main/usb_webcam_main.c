#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_ota_ops.h"
#include "esp_http_server.h"
#include "rom/ets_sys.h"
#include <inttypes.h> 
#include "esp_camera.h"
#include "usb_device_uvc.h"
#include "uvc_frame_config.h"

static const char *TAG = "XIAO_Webcam_OTA_AP";

#define LED_GPIO 21
#define UVC_MAX_FRAMESIZE_SIZE (128U * 1024U)
#define CAMERA_FB_COUNT 2

/* Access Point credentials */
#define AP_SSID      "XIAO_AP"
#define AP_PASSWORD  "12345678"
#define AP_CHANNEL   1
#define MAX_STA_CONN 4  // Max stations that can connect

typedef struct
{
    camera_fb_t *cam_fb_p;
    uvc_fb_t uvc_fb;
} fb_t;

static fb_t s_fb;

/*******************************************************************
 * Camera and UVC code (unchanged except for a 10s delay at start)
 *******************************************************************/
static esp_err_t camera_init(uint32_t xclk_freq_hz, pixformat_t pixel_format, framesize_t frame_size, int jpeg_quality, uint8_t fb_count)
{
    static bool inited = false;
    static uint32_t cur_xclk_freq_hz = 0;
    static pixformat_t cur_pixel_format = 0;
    static framesize_t cur_frame_size = 0;
    static uint8_t cur_fb_count = 0;
    static int cur_jpeg_quality = 0;

    if ((inited && cur_xclk_freq_hz == xclk_freq_hz && cur_pixel_format == pixel_format && cur_frame_size == frame_size
         && cur_fb_count == fb_count && cur_jpeg_quality == jpeg_quality))
    {
        ESP_LOGD(TAG, "camera already inited");
        return ESP_OK;
    }
    else if (inited)
    {
        esp_camera_return_all();
        esp_camera_deinit();
        inited = false;
        ESP_LOGI(TAG, "camera RESTART");
    }

    camera_config_t camera_config = {
        .pin_pwdn  = CONFIG_CAMERA_PIN_PWDN,
        .pin_reset = CONFIG_CAMERA_PIN_RESET,
        .pin_xclk  = CONFIG_CAMERA_PIN_XCLK,
        .pin_sscb_sda = CONFIG_CAMERA_PIN_SIOD,
        .pin_sscb_scl = CONFIG_CAMERA_PIN_SIOC,

        .pin_d7 = CONFIG_CAMERA_PIN_Y9,
        .pin_d6 = CONFIG_CAMERA_PIN_Y8,
        .pin_d5 = CONFIG_CAMERA_PIN_Y7,
        .pin_d4 = CONFIG_CAMERA_PIN_Y6,
        .pin_d3 = CONFIG_CAMERA_PIN_Y5,
        .pin_d2 = CONFIG_CAMERA_PIN_Y4,
        .pin_d1 = CONFIG_CAMERA_PIN_Y3,
        .pin_d0 = CONFIG_CAMERA_PIN_Y2,
        .pin_vsync = CONFIG_CAMERA_PIN_VSYNC,
        .pin_href  = CONFIG_CAMERA_PIN_HREF,
        .pin_pclk  = CONFIG_CAMERA_PIN_PCLK,

        .xclk_freq_hz = xclk_freq_hz,
        .ledc_timer   = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = pixel_format,
        .frame_size   = frame_size,
        .jpeg_quality = jpeg_quality,
        .fb_count     = fb_count,
        .grab_mode    = CAMERA_GRAB_WHEN_EMPTY,
        .fb_location  = CAMERA_FB_IN_PSRAM
    };

    esp_err_t ret = esp_camera_init(&camera_config);
    if (ret != ESP_OK) {
        return ret;
    }

    sensor_t *s = esp_camera_sensor_get();
    camera_sensor_info_t *s_info = esp_camera_sensor_get_info(&(s->id));

    if ((ret == ESP_OK) && (PIXFORMAT_JPEG == pixel_format) && (s_info->support_jpeg == true)) {
        cur_xclk_freq_hz = xclk_freq_hz;
        cur_pixel_format  = pixel_format;
        cur_frame_size    = frame_size;
        cur_jpeg_quality  = jpeg_quality;
        cur_fb_count      = fb_count;
        inited = true;
    } else {
        ESP_LOGE(TAG, "JPEG format is not supported");
        return ESP_ERR_NOT_SUPPORTED;
    }

    return ret;
}

static void camera_stop_cb(void *cb_ctx)
{
    (void)cb_ctx;
    gpio_set_level(LED_GPIO, 0x01);
    ESP_LOGI(TAG, "Camera Stop");
}

static esp_err_t camera_start_cb(uvc_format_t format, int width, int height, int rate, void *cb_ctx)
{
    (void)cb_ctx;
    ESP_LOGI(TAG, "Camera Start");
    ESP_LOGI(TAG, "Format: %d, width: %d, height: %d, rate: %d", format, width, height, rate);

    framesize_t frame_size = FRAMESIZE_QVGA;
    int jpeg_quality = 14;

    if (format != UVC_FORMAT_JPEG) {
        ESP_LOGE(TAG, "Only support MJPEG format");
        return ESP_ERR_NOT_SUPPORTED;
    }

    if (width == 320 && height == 240) {
        frame_size = FRAMESIZE_QVGA;
        jpeg_quality = 10;
    } else if (width == 480 && height == 320) {
        frame_size = FRAMESIZE_HVGA;
        jpeg_quality = 10;
    } else if (width == 640 && height == 480) {
        frame_size = FRAMESIZE_VGA;
        jpeg_quality = 12;
    } else if (width == 800 && height == 600) {
        frame_size = FRAMESIZE_SVGA;
        jpeg_quality = 14;
    } else if (width == 1280 && height == 720) {
        frame_size = FRAMESIZE_HD;
        jpeg_quality = 16;
    } else if (width == 1920 && height == 1080) {
        frame_size = FRAMESIZE_FHD;
        jpeg_quality = 16;
    } else {
        ESP_LOGE(TAG, "Unsupported frame size %dx%d", width, height);
        return ESP_ERR_NOT_SUPPORTED;
    }

    esp_err_t ret = camera_init(CONFIG_CAMERA_XCLK_FREQ, PIXFORMAT_JPEG, frame_size, jpeg_quality, CAMERA_FB_COUNT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "camera init failed");
        return ret;
    }

    // Turn off LED to indicate camera streaming
    gpio_set_level(LED_GPIO, 0x00);
    return ESP_OK;
}

static uvc_fb_t *camera_fb_get_cb(void *cb_ctx)
{
    (void)cb_ctx;
    s_fb.cam_fb_p = esp_camera_fb_get();
    if (!s_fb.cam_fb_p) {
        return NULL;
    }
    s_fb.uvc_fb.buf       = s_fb.cam_fb_p->buf;
    s_fb.uvc_fb.len       = s_fb.cam_fb_p->len;
    s_fb.uvc_fb.width     = s_fb.cam_fb_p->width;
    s_fb.uvc_fb.height    = s_fb.cam_fb_p->height;
    s_fb.uvc_fb.format    = s_fb.cam_fb_p->format;
    s_fb.uvc_fb.timestamp = s_fb.cam_fb_p->timestamp;

    if (s_fb.uvc_fb.len > UVC_MAX_FRAMESIZE_SIZE) {
        ESP_LOGE(TAG, "Frame size %d > max %d", s_fb.uvc_fb.len, UVC_MAX_FRAMESIZE_SIZE);
        esp_camera_fb_return(s_fb.cam_fb_p);
        return NULL;
    }
    return &s_fb.uvc_fb;
}

static void camera_fb_return_cb(uvc_fb_t *fb, void *cb_ctx)
{
    (void)cb_ctx;
    assert(fb == &s_fb.uvc_fb);
    esp_camera_fb_return(s_fb.cam_fb_p);
}

/*******************************************************************
 * OTA Upload Handlers + Basic Web Form
 *******************************************************************/
static const char *OTA_TAG = "OTA_HTTP";

// A simple HTML page with a file-upload form for OTA:
static const char *UPLOAD_PAGE =
"<!DOCTYPE html>"
"<html>"
"<head><title>OTA Update</title></head>"
"<body>"
"<h1>OTA Update</h1>"
"<form method=\"POST\" action=\"/upload\" enctype=\"multipart/form-data\">"
"<input type=\"file\" name=\"file\">"
"<input type=\"submit\" value=\"Upload\">"
"</form>"
"</body>"
"</html>";

// / handler: just send our upload form
static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_send(req, UPLOAD_PAGE, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static const httpd_uri_t root = {
    .uri     = "/",
    .method  = HTTP_GET,
    .handler = root_get_handler
};

// /upload POST handler: receive the uploaded binary and flash OTA
static esp_err_t upload_post_handler(httpd_req_t *req)
{
    const esp_partition_t *ota_partition = esp_ota_get_next_update_partition(NULL);
    if (!ota_partition) {
        ESP_LOGE(OTA_TAG, "No OTA partition found");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No OTA partition found");
        return ESP_FAIL;
    }


    ESP_LOGI(OTA_TAG, "Begin OTA to partition subtype %d at offset 0x%" PRIu32,
        ota_partition->subtype, ota_partition->address);

    esp_ota_handle_t ota_handle;
    esp_err_t err = esp_ota_begin(ota_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(OTA_TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA begin failed");
        return ESP_FAIL;
    }

    // Read the incoming file data in small chunks
    char buf[2048];
    int total_received = 0;
    int remaining = req->content_len;
    while (remaining > 0) {
        int to_read = (remaining < (int)sizeof(buf)) ? remaining : (int)sizeof(buf);
        int ret = httpd_req_recv(req, buf, to_read);
        if (ret <= 0) {
            ESP_LOGE(OTA_TAG, "httpd_req_recv failed: ret=%d", ret);
            esp_ota_end(ota_handle);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Upload error");
            return ESP_FAIL;
        }
        err = esp_ota_write(ota_handle, buf, ret);
        if (err != ESP_OK) {
            ESP_LOGE(OTA_TAG, "esp_ota_write failed (%s)", esp_err_to_name(err));
            esp_ota_end(ota_handle);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA write failed");
            return ESP_FAIL;
        }
        total_received += ret;
        remaining      -= ret;
    }

    ESP_LOGI(OTA_TAG, "OTA uploaded size: %d bytes", total_received);
    if (esp_ota_end(ota_handle) == ESP_OK) {
        err = esp_ota_set_boot_partition(ota_partition);
        if (err == ESP_OK) {
            ESP_LOGI(OTA_TAG, "OTA done, rebooting...");
            httpd_resp_sendstr(req, "Firmware updated. Rebooting in 2s...");
            vTaskDelay(pdMS_TO_TICKS(2000));
            esp_restart();
            return ESP_OK; // unreachable
        } else {
            ESP_LOGE(OTA_TAG, "esp_ota_set_boot_partition failed (%s)", esp_err_to_name(err));
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Set boot partition failed");
            return ESP_FAIL;
        }
    } else {
        ESP_LOGE(OTA_TAG, "esp_ota_end failed");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA end failed");
        return ESP_FAIL;
    }
}

static const httpd_uri_t upload = {
    .uri     = "/upload",
    .method  = HTTP_POST,
    .handler = upload_post_handler
};

// Start an HTTP server on port 80 with two URIs: "/" for the form and "/upload" for the OTA
static httpd_handle_t start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn   = httpd_uri_match_wildcard;
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &upload);
    }
    return server;
}

/*******************************************************************
 * Access Point Setup
 *******************************************************************/
static void wifi_init_softap(void)
{
    ESP_LOGI(TAG, "Initializing AP \"%s\"...", AP_SSID);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_init_config_t netif_cfg = ESP_NETIF_DEFAULT_WIFI_AP();
    esp_netif_t *netif = esp_netif_create_default_wifi_ap();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .channel = AP_CHANNEL,
            .password = AP_PASSWORD,
            .max_connection = MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(AP_PASSWORD) == 0) {
        wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // IP is usually 192.168.4.1 for default AP mode
    ESP_LOGI(TAG, "SoftAP started. SSID: %s password:%s channel:%d",
             AP_SSID, AP_PASSWORD, AP_CHANNEL);
}

/*******************************************************************
 * Main Application
 *******************************************************************/
void app_main(void)
{
    // Delay so we can press BOOT / do a flash erase if needed
    vTaskDelay(pdMS_TO_TICKS(10000));

    // LED setup (on many boards GPIO 21 is an LED or unused pin)
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0x01); // Turn on LED at start

    // Initialize NVS (required by Wi-Fi and OTA)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Erase and re-init
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // Create our AP
    wifi_init_softap();

    // Start webserver for OTA
    start_webserver();
    ESP_LOGI(TAG, "OTA webserver available at http://192.168.4.1/");

    // Allocate UVC buffer and init camera
    uint8_t *uvc_buffer = (uint8_t *)malloc(UVC_MAX_FRAMESIZE_SIZE);
    if (!uvc_buffer) {
        ESP_LOGE(TAG, "malloc frame buffer fail");
        return;
    }

    uvc_device_config_t config = {
        .uvc_buffer      = uvc_buffer,
        .uvc_buffer_size = UVC_MAX_FRAMESIZE_SIZE,
        .start_cb        = camera_start_cb,
        .fb_get_cb       = camera_fb_get_cb,
        .fb_return_cb    = camera_fb_return_cb,
        .stop_cb         = camera_stop_cb,
    };

    ESP_LOGI(TAG, "UVC Format List");
    ESP_LOGI(TAG, "\tFormat(1) = MJPEG");
    ESP_LOGI(TAG, "Frame List");
    ESP_LOGI(TAG, "\tFrame(1) = %d * %d @%dfps", UVC_FRAMES_INFO[0][0].width,
               UVC_FRAMES_INFO[0][0].height, UVC_FRAMES_INFO[0][0].rate);

#if CONFIG_CAMERA_MULTI_FRAMESIZE
    ESP_LOGI(TAG, "\tFrame(2) = %d * %d @%dfps", UVC_FRAMES_INFO[0][1].width,
               UVC_FRAMES_INFO[0][1].height, UVC_FRAMES_INFO[0][1].rate);
    ESP_LOGI(TAG, "\tFrame(3) = %d * %d @%dfps", UVC_FRAMES_INFO[0][2].width,
               UVC_FRAMES_INFO[0][2].height, UVC_FRAMES_INFO[0][2].rate);
    ESP_LOGI(TAG, "\tFrame(4) = %d * %d @%dfps", UVC_FRAMES_INFO[0][3].width,
               UVC_FRAMES_INFO[0][3].height, UVC_FRAMES_INFO[0][3].rate);
#endif

    ESP_ERROR_CHECK(uvc_device_config(0, &config));
    ESP_ERROR_CHECK(uvc_device_init());

    // Main loop does nothing. UVC streaming + OTA webserver run in background tasks
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        // Could blink an LED, check memory, etc.
    }
}
