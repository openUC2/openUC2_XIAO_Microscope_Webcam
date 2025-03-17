#include "esp_camera.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "usb_device_uvc.h"
#include "uvc_frame_config.h"

static const char *TAG = "XIAO_Webcam";

#define LED_GPIO 21
#define UVC_MAX_FRAMESIZE_SIZE (128U * 1024U)
#define CAMERA_FB_COUNT 2

typedef struct
{
    camera_fb_t *cam_fb_p;
    uvc_fb_t uvc_fb;
} fb_t;

static fb_t s_fb;

static esp_err_t camera_init(uint32_t xclk_freq_hz, pixformat_t pixel_format, framesize_t frame_size, int jpeg_quality, uint8_t fb_count)
{
    static bool inited = false;
    static uint32_t cur_xclk_freq_hz = 0;
    static pixformat_t cur_pixel_format = 0;
    static framesize_t cur_frame_size = 0;
    static uint8_t cur_fb_count = 0;
    static int cur_jpeg_quality = 0;

    if ((inited && cur_xclk_freq_hz == xclk_freq_hz && cur_pixel_format == pixel_format && cur_frame_size == frame_size && cur_fb_count == fb_count && cur_jpeg_quality == jpeg_quality))
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
        .pin_pwdn = CONFIG_CAMERA_PIN_PWDN,
        .pin_reset = CONFIG_CAMERA_PIN_RESET,
        .pin_xclk = CONFIG_CAMERA_PIN_XCLK,
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
        .pin_href = CONFIG_CAMERA_PIN_HREF,
        .pin_pclk = CONFIG_CAMERA_PIN_PCLK,

        .xclk_freq_hz = xclk_freq_hz,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = pixel_format,
        .frame_size = frame_size,

        .jpeg_quality = jpeg_quality,
        .fb_count = fb_count,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
        .fb_location = CAMERA_FB_IN_PSRAM};

    esp_err_t ret = esp_camera_init(&camera_config);

    if (ret != ESP_OK)
        return ret;

    sensor_t *s = esp_camera_sensor_get();
    camera_sensor_info_t *s_info = esp_camera_sensor_get_info(&(s->id));

    if (ESP_OK == ret && PIXFORMAT_JPEG == pixel_format && s_info->support_jpeg == true)
    {
        cur_xclk_freq_hz = xclk_freq_hz;
        cur_pixel_format = pixel_format;
        cur_frame_size = frame_size;
        cur_jpeg_quality = jpeg_quality;
        cur_fb_count = fb_count;
        inited = true;
    }
    else
    {
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

    if (format != UVC_FORMAT_JPEG)
    {
        ESP_LOGE(TAG, "Only support MJPEG format");

        return ESP_ERR_NOT_SUPPORTED;
    }

    if (width == 320 && height == 240)
    {
        frame_size = FRAMESIZE_QVGA;
        jpeg_quality = 10;
    }
    else if (width == 480 && height == 320)
    {
        frame_size = FRAMESIZE_HVGA;
        jpeg_quality = 10;
    }
    else if (width == 640 && height == 480)
    {
        frame_size = FRAMESIZE_VGA;
        jpeg_quality = 12;
    }
    else if (width == 800 && height == 600)
    {
        frame_size = FRAMESIZE_SVGA;
        jpeg_quality = 14;
    }
    else if (width == 1280 && height == 720)
    {
        frame_size = FRAMESIZE_HD;
        jpeg_quality = 16;
    }
    else if (width == 1920 && height == 1080)
    {
        frame_size = FRAMESIZE_FHD;
        jpeg_quality = 16;
    }
    else
    {
        ESP_LOGE(TAG, "Unsupported frame size %dx%d", width, height);

        return ESP_ERR_NOT_SUPPORTED;
    }

    esp_err_t ret = camera_init(CONFIG_CAMERA_XCLK_FREQ, PIXFORMAT_JPEG, frame_size, jpeg_quality, CAMERA_FB_COUNT);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "camera init failed");

        return ret;
    }

    gpio_set_level(LED_GPIO, 0x00);

    return ESP_OK;
}

static uvc_fb_t *camera_fb_get_cb(void *cb_ctx)
{
    (void)cb_ctx;

    s_fb.cam_fb_p = esp_camera_fb_get();

    if (!s_fb.cam_fb_p)
        return NULL;

    s_fb.uvc_fb.buf = s_fb.cam_fb_p->buf;
    s_fb.uvc_fb.len = s_fb.cam_fb_p->len;
    s_fb.uvc_fb.width = s_fb.cam_fb_p->width;
    s_fb.uvc_fb.height = s_fb.cam_fb_p->height;
    s_fb.uvc_fb.format = s_fb.cam_fb_p->format;
    s_fb.uvc_fb.timestamp = s_fb.cam_fb_p->timestamp;

    if (s_fb.uvc_fb.len > UVC_MAX_FRAMESIZE_SIZE)
    {
        ESP_LOGE(TAG, "Frame size %d is larger than max frame size %d", s_fb.uvc_fb.len, UVC_MAX_FRAMESIZE_SIZE);

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

void app_main(void)
{
    // add  delay to have abbility to erase flash and add a new firmware
    vTaskDelay(pdMS_TO_TICKS(10000));

    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0x01);


    uint8_t *uvc_buffer = (uint8_t *)malloc(UVC_MAX_FRAMESIZE_SIZE);

    if (!uvc_buffer)
    {
        ESP_LOGE(TAG, "malloc frame buffer fail");

        return;
    }

    uvc_device_config_t config = {
        .uvc_buffer = uvc_buffer,
        .uvc_buffer_size = UVC_MAX_FRAMESIZE_SIZE,
        .start_cb = camera_start_cb,
        .fb_get_cb = camera_fb_get_cb,
        .fb_return_cb = camera_fb_return_cb,
        .stop_cb = camera_stop_cb,
    };

    ESP_LOGI(TAG, "Format List");
    ESP_LOGI(TAG, "\tFormat(1) = %s", "MJPEG");
    ESP_LOGI(TAG, "Frame List");
    ESP_LOGI(TAG, "\tFrame(1) = %d * %d @%dfps", UVC_FRAMES_INFO[0][0].width, UVC_FRAMES_INFO[0][0].height, UVC_FRAMES_INFO[0][0].rate);
#if CONFIG_CAMERA_MULTI_FRAMESIZE
    ESP_LOGI(TAG, "\tFrame(2) = %d * %d @%dfps", UVC_FRAMES_INFO[0][1].width, UVC_FRAMES_INFO[0][1].height, UVC_FRAMES_INFO[0][1].rate);
    ESP_LOGI(TAG, "\tFrame(3) = %d * %d @%dfps", UVC_FRAMES_INFO[0][2].width, UVC_FRAMES_INFO[0][2].height, UVC_FRAMES_INFO[0][2].rate);
    ESP_LOGI(TAG, "\tFrame(3) = %d * %d @%dfps", UVC_FRAMES_INFO[0][3].width, UVC_FRAMES_INFO[0][3].height, UVC_FRAMES_INFO[0][3].rate);
#endif

    ESP_ERROR_CHECK(uvc_device_config(0, &config));
    ESP_ERROR_CHECK(uvc_device_init());

    while (true)
        vTaskDelay(pdMS_TO_TICKS(100));
}
