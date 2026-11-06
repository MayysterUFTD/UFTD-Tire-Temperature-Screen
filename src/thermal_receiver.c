#include "thermal_receiver.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "lvgl.h"
#include "nvs_flash.h"
#include "wdcp_fragment.h"
#include "wdcp_protocol.h"

#include "lv_port.h"
#include "ui/screens.h"

#define THERMAL_WIDTH 32
#define THERMAL_HEIGHT 24
#define THERMAL_ZONES 3
#define THERMAL_PIXELS (THERMAL_WIDTH * THERMAL_HEIGHT)
#define THERMAL_HEADER_SIZE 6
#define THERMAL_FRAME_SIZE (THERMAL_HEADER_SIZE + THERMAL_PIXELS)
#define THERMAL_SOURCE_ID 0x0301
#define ESPNOW_CHANNEL 1
#define ESPNOW_RX_QUEUE_LENGTH 16
#define DISPLAY_WIDTH 131
#define DISPLAY_HEIGHT 150

static const char *TAG = "thermal_receiver";

typedef struct
{
    uint16_t len;
    uint8_t bytes[WDCP_MAX_FRAME_LEN];
} received_packet_t;

static QueueHandle_t rx_queue;
static wdcp_frag_rx_t fragment_rx;
static volatile uint32_t rx_queue_drops;
static uint8_t thermal_pixels[THERMAL_PIXELS];
static bool thermal_frame_pending;
static uint32_t thermal_frames_received;
static lv_obj_t *thermal_canvas;
static lv_obj_t *zone_labels[THERMAL_ZONES];
static lv_obj_t *zone_shadows[THERMAL_ZONES];
static lv_obj_t *max_label;
static lv_obj_t *max_shadow;
static uint16_t *thermal_canvas_buffer;

static void on_thermal_frame(uint16_t source_id, const uint8_t *data, uint16_t len, void *user_data)
{
    (void)user_data;
    if (source_id != THERMAL_SOURCE_ID || len != THERMAL_FRAME_SIZE ||
        data[0] != 'T' || data[1] != 'H' || data[2] != 1 ||
        data[3] != THERMAL_WIDTH || data[4] != THERMAL_HEIGHT || data[5] != 2)
    {
        ESP_LOGW(TAG, "Discarded invalid thermal frame");
        return;
    }

    for (size_t i = 0; i < THERMAL_PIXELS; ++i)
    {
        thermal_pixels[i] = data[THERMAL_HEADER_SIZE + i];
    }
    ++thermal_frames_received;
    thermal_frame_pending = true;
}

static void espnow_receive_callback(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    (void)info;
    if (!rx_queue || !data || len <= 0 || len > WDCP_MAX_FRAME_LEN)
    {
        return;
    }

    received_packet_t packet = {.len = (uint16_t)len};
    memcpy(packet.bytes, data, (size_t)len);
    if (xQueueSend(rx_queue, &packet, 0) != pdTRUE)
    {
        ++rx_queue_drops;
    }
}

static void initialize_wifi_and_espnow(void)
{
    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        result = nvs_flash_init();
    }
    ESP_ERROR_CHECK(result);

    ESP_ERROR_CHECK(esp_netif_init());
    result = esp_event_loop_create_default();
    if (result != ESP_OK && result != ESP_ERR_INVALID_STATE)
    {
        ESP_ERROR_CHECK(result);
    }
    if (esp_netif_create_default_wifi_sta() == NULL)
    {
        ESP_ERROR_CHECK(ESP_ERR_NO_MEM);
    }

    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_receive_callback));
}

static uint16_t rgb565_swapped(uint8_t red, uint8_t green, uint8_t blue)
{
    uint16_t rgb565 = ((uint16_t)(red & 0xF8) << 8) |
                      ((uint16_t)(green & 0xFC) << 3) |
                      ((uint16_t)blue >> 3);
    return (uint16_t)((rgb565 << 8) | (rgb565 >> 8));
}

static uint16_t thermal_color(int32_t value, int32_t min_value, int32_t max_value)
{
    uint8_t level = max_value == min_value
                        ? 128
                        : (uint8_t)(((value - min_value) * 255) / (max_value - min_value));
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    if (level < 64)
    {
        red = 0;
        green = (uint8_t)(level * 4);
        blue = 255;
    }
    else if (level < 128)
    {
        red = 0;
        green = 255;
        blue = (uint8_t)(255 - (level - 64) * 4);
    }
    else if (level < 192)
    {
        red = (uint8_t)((level - 128) * 4);
        green = 255;
        blue = 0;
    }
    else
    {
        red = 255;
        green = (uint8_t)(255 - (level - 192) * 4);
        blue = 0;
    }

    return rgb565_swapped(red, green, blue);
}

static void set_label_pair(lv_obj_t *label, lv_obj_t *shadow, const char *text)
{
    lv_label_set_text(label, text);
    lv_label_set_text(shadow, text);
}

static void render_thermal_frame(void)
{
    int32_t min_value = INT32_MAX;
    int32_t max_value = INT32_MIN;
    for (size_t i = 0; i < THERMAL_PIXELS; ++i)
    {
        if (thermal_pixels[i] == UINT8_MAX)
        {
            continue;
        }
        int32_t temperature = (int32_t)thermal_pixels[i] - 40;
        if (temperature < min_value)
        {
            min_value = temperature;
        }
        if (temperature > max_value)
        {
            max_value = temperature;
        }
    }

    if (min_value == INT32_MAX)
    {
        lv_obj_set_hidden(thermal_canvas, true);
        for (int zone = 0; zone < THERMAL_ZONES; ++zone)
        {
            set_label_pair(zone_labels[zone], zone_shadows[zone], "--");
        }
        set_label_pair(max_label, max_shadow, "--");
        return;
    }

    uint16_t palette[256];
    for (int32_t level = 0; level < 256; ++level)
    {
        palette[level] = thermal_color(min_value + level * (max_value - min_value) / 255, min_value, max_value);
    }
    uint16_t black = rgb565_swapped(0, 0, 0);

    static uint16_t row_buffer[DISPLAY_WIDTH];
    static uint16_t column_source[DISPLAY_WIDTH];
    if (column_source[DISPLAY_WIDTH - 1] == 0)
    {
        for (int x = 0; x < DISPLAY_WIDTH; ++x)
        {
            column_source[x] = (uint16_t)((x * THERMAL_WIDTH) / DISPLAY_WIDTH) + 1;
        }
    }

    int32_t range = max_value - min_value;
    int previous_source_y = -1;
    for (int y = 0; y < DISPLAY_HEIGHT; ++y)
    {
        int source_y = (y * THERMAL_HEIGHT) / DISPLAY_HEIGHT;
        uint16_t *destination = &thermal_canvas_buffer[y * DISPLAY_WIDTH];
        if (source_y == previous_source_y)
        {
            memcpy(destination, destination - DISPLAY_WIDTH, sizeof(row_buffer));
            continue;
        }
        previous_source_y = source_y;

        uint16_t row_colors[THERMAL_WIDTH];
        for (int sx = 0; sx < THERMAL_WIDTH; ++sx)
        {
            uint8_t pixel = thermal_pixels[source_y * THERMAL_WIDTH + sx];
            if (pixel == UINT8_MAX)
            {
                row_colors[sx] = black;
            }
            else
            {
                int32_t temperature = (int32_t)pixel - 40;
                row_colors[sx] = range == 0 ? palette[128] : palette[((temperature - min_value) * 255) / range];
            }
        }
        for (int x = 0; x < DISPLAY_WIDTH; ++x)
        {
            row_buffer[x] = row_colors[column_source[x] - 1];
        }
        memcpy(destination, row_buffer, sizeof(row_buffer));
    }

    char text[16];
    for (int zone = 0; zone < THERMAL_ZONES; ++zone)
    {
        int32_t sum = 0;
        int32_t count = 0;
        for (int y = 0; y < THERMAL_HEIGHT; ++y)
        {
            for (int x = 0; x < THERMAL_WIDTH; ++x)
            {
                uint8_t pixel = thermal_pixels[y * THERMAL_WIDTH + x];
                if ((x * THERMAL_ZONES) / THERMAL_WIDTH == zone && pixel != UINT8_MAX)
                {
                    sum += (int32_t)pixel - 40;
                    ++count;
                }
            }
        }
        if (count == 0)
        {
            set_label_pair(zone_labels[zone], zone_shadows[zone], "--");
            continue;
        }
        int32_t average = sum >= 0 ? (sum + count / 2) / count : (sum - count / 2) / count;
        snprintf(text, sizeof(text), "%ld", (long)average);
        set_label_pair(zone_labels[zone], zone_shadows[zone], text);
    }
    snprintf(text, sizeof(text), "%ld", (long)max_value);
    set_label_pair(max_label, max_shadow, text);

    lv_obj_set_hidden(thermal_canvas, false);
    lv_obj_invalidate(thermal_canvas);
}

void thermal_receiver_init(void)
{
    rx_queue = xQueueCreate(ESPNOW_RX_QUEUE_LENGTH, sizeof(received_packet_t));
    if (!rx_queue)
    {
        ESP_ERROR_CHECK(ESP_ERR_NO_MEM);
    }
    wdcp_frag_rx_init(&fragment_rx, 500);
    initialize_wifi_and_espnow();

    thermal_canvas_buffer = heap_caps_calloc(
        DISPLAY_WIDTH * DISPLAY_HEIGHT, sizeof(uint16_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!thermal_canvas_buffer)
    {
        ESP_LOGE(TAG, "Could not allocate thermal overlay buffer");
        ESP_ERROR_CHECK(ESP_ERR_NO_MEM);
    }

    if (!objects.fl_tire || !lvgl_port_lock(0))
    {
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    thermal_canvas = lv_canvas_create(objects.fl_tire);
    if (!thermal_canvas)
    {
        lvgl_port_unlock();
        ESP_ERROR_CHECK(ESP_ERR_NO_MEM);
    }
    lv_canvas_set_buffer(thermal_canvas, thermal_canvas_buffer, DISPLAY_WIDTH, DISPLAY_HEIGHT,
                         LV_COLOR_FORMAT_RGB565_SWAPPED);
    lv_obj_set_pos(thermal_canvas, 0, 0);
    lv_obj_set_size(thermal_canvas, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_remove_flag(thermal_canvas, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_hidden(thermal_canvas, true);

    // The image sits behind the widget's labels; the zone panels become see-through.
    lv_obj_move_background(thermal_canvas);
    lv_obj_t *zone_panels[THERMAL_ZONES] = {objects.fl_tire__obj0, objects.fl_tire__obj3, objects.fl_tire__obj6};
    for (int zone = 0; zone < THERMAL_ZONES; ++zone)
    {
        lv_obj_set_style_bg_opa(zone_panels[zone], LV_OPA_TRANSP, LV_PART_MAIN);
    }
    zone_labels[0] = objects.fl_tire__obj1;
    zone_shadows[0] = objects.fl_tire__obj2;
    zone_labels[1] = objects.fl_tire__obj4;
    zone_shadows[1] = objects.fl_tire__obj5;
    zone_labels[2] = objects.fl_tire__obj7;
    zone_shadows[2] = objects.fl_tire__obj8;
    max_label = objects.fl_tire__obj9;
    max_shadow = objects.fl_tire__obj10;

    // Centre the large label across the whole widget; the shadow copy follows it.
    lv_obj_set_pos(max_label, 0, 48);
    lv_obj_set_width(max_label, DISPLAY_WIDTH);
    lv_obj_set_width(max_shadow, DISPLAY_WIDTH);
    for (int zone = 0; zone < THERMAL_ZONES; ++zone)
    {
        set_label_pair(zone_labels[zone], zone_shadows[zone], "--");
    }
    set_label_pair(max_label, max_shadow, "--");
    lvgl_port_unlock();

    ESP_LOGI(TAG, "WDCP thermal receiver ready on ESP-NOW channel %d", ESPNOW_CHANNEL);
}

void thermal_receiver_process(void)
{
    received_packet_t packet;
    uint32_t now_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);

    while (xQueueReceive(rx_queue, &packet, 0) == pdTRUE)
    {
        wdcp_frame_t frame;
        if (!wdcp_decode_frame(packet.bytes, packet.len, &frame) ||
            frame.dst_id != WDCP_NODE_BROADCAST ||
            frame.src_id != THERMAL_SOURCE_ID ||
            !(frame.flags & WDCP_FLAG_FRAGMENT))
        {
            continue;
        }
        wdcp_frag_rx_process(&fragment_rx, &frame, now_ms, on_thermal_frame, NULL);
    }

    wdcp_frag_rx_check_timeouts(&fragment_rx, now_ms);

    static uint32_t reported_drops;
    uint32_t current_drops = rx_queue_drops;
    if (current_drops != reported_drops)
    {
        ESP_LOGW(TAG, "Dropped %u ESP-NOW fragments because the RX queue was full",
                 (unsigned)(current_drops - reported_drops));
        reported_drops = current_drops;
    }

    if (thermal_frame_pending && lvgl_port_lock(40))
    {
        int64_t start_us = esp_timer_get_time();
        render_thermal_frame();
        thermal_frame_pending = false;
        lvgl_port_unlock();

        static uint32_t frames;
        static int64_t render_total_us;
        static int64_t window_start_us;
        render_total_us += esp_timer_get_time() - start_us;
        ++frames;
        if (window_start_us == 0)
        {
            window_start_us = start_us;
        }
        else if (start_us - window_start_us >= 3000000)
        {
            ESP_LOGI(TAG, "%.1f fps shown, %u frames received, render %lld ms/frame",
                     frames * 1000000.0f / (float)(start_us - window_start_us),
                     (unsigned)thermal_frames_received,
                     (long long)(render_total_us / frames / 1000));
            thermal_frames_received = 0;
            frames = 0;
            render_total_us = 0;
            window_start_us = start_us;
        }
    }
}
