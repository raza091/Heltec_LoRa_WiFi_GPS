// File: main/Heltec_LoRa_WiFi_GPS.c
// Heltec WiFi LoRa 32 V3 + NEO-7M GPS + OLED
// Improved GGA parser for correct Lat/Lon extraction

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/i2c_master.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "ssd1306.h"

// ==================== CONFIG ====================
#define GPS_UART_PORT       UART_NUM_1
#define GPS_TX_PIN          GPIO_NUM_46
#define GPS_RX_PIN          GPIO_NUM_45
#define GPS_BAUDRATE        9600

#define VEXT_GPIO           GPIO_NUM_36
#define OLED_RESET_GPIO     GPIO_NUM_21

static SSD1306_t dev;

// ==================== GPS DATA ====================
typedef struct {
    bool valid_fix;
    float latitude;
    float longitude;
    uint8_t satellites;
    char utc_time[16];
    uint32_t last_update_ms;
} gps_data_t;

static gps_data_t gps_info = {0};

// ==================== IMPROVED NMEA GGA PARSER ====================
static void parse_nmea(const char *sentence)
{
    if (strstr(sentence, "$GPGGA") == NULL && strstr(sentence, "$GNGGA") == NULL) {
        return;
    }

    // Tokenize the sentence
    char temp[128];
    strncpy(temp, sentence, sizeof(temp)-1);
    temp[sizeof(temp)-1] = '\0';

    char *token = strtok(temp, ",");
    if (!token) return;

    // Field 1: UTC Time
    token = strtok(NULL, ",");
    if (!token) return;
    strncpy(gps_info.utc_time, token, 9);
    gps_info.utc_time[9] = '\0';

    // Field 2: Latitude (ddmm.mmmm)
    token = strtok(NULL, ",");
    if (!token) return;
    float lat_raw = atof(token);

    // Field 3: N/S
    token = strtok(NULL, ",");
    if (!token) return;
    char lat_dir = token[0];

    // Field 4: Longitude (dddmm.mmmm)
    token = strtok(NULL, ",");
    if (!token) return;
    float lon_raw = atof(token);

    // Field 5: E/W
    token = strtok(NULL, ",");
    if (!token) return;
    char lon_dir = token[0];

    // Field 6: Fix quality
    token = strtok(NULL, ",");
    if (!token) return;
    int fix = atoi(token);

    // Field 7: Satellites
    token = strtok(NULL, ",");
    if (!token) return;
    gps_info.satellites = atoi(token);

    // Update status
    gps_info.valid_fix = (fix >= 1);
    gps_info.last_update_ms = esp_timer_get_time() / 1000;

    // Convert to decimal degrees
    if (lat_raw > 0) {
        float lat_deg = (int)(lat_raw / 100);
        float lat_min = lat_raw - lat_deg * 100;
        gps_info.latitude = lat_deg + lat_min / 60.0f;
        if (lat_dir == 'S') gps_info.latitude = -gps_info.latitude;
    }

    if (lon_raw > 0) {
        float lon_deg = (int)(lon_raw / 100);
        float lon_min = lon_raw - lon_deg * 100;
        gps_info.longitude = lon_deg + lon_min / 60.0f;
        if (lon_dir == 'W') gps_info.longitude = -gps_info.longitude;
    }
}

// ==================== GPS UART TASK ====================
static void gps_uart_task(void *arg)
{
    uart_config_t uart_config = {
        .baud_rate = GPS_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_driver_install(GPS_UART_PORT, 1024, 0, 0, NULL, 0);
    uart_param_config(GPS_UART_PORT, &uart_config);
    uart_set_pin(GPS_UART_PORT, GPS_TX_PIN, GPS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uint8_t data[256];
    char line[128];
    int pos = 0;

    while (1) {
        int len = uart_read_bytes(GPS_UART_PORT, data, sizeof(data)-1, pdMS_TO_TICKS(100));
        if (len <= 0) continue;

        for (int i = 0; i < len; i++) {
            char c = data[i];
            if (c == '\n' || c == '\r') {
                if (pos > 5 && line[0] == '$') {
                    line[pos] = '\0';
                    parse_nmea(line);
                }
                pos = 0;
            } else if (pos < sizeof(line)-1) {
                line[pos++] = c;
            }
        }
    }
}

// ==================== OLED DISPLAY ====================
static void display_gps(void)
{
    char buf[32];

    ssd1306_clear_screen(&dev, false);

    sprintf(buf, "GPS NEO-7M");
    ssd1306_display_text(&dev, 0, buf, strlen(buf), false);

    if (gps_info.valid_fix && gps_info.latitude != 0.0f) {
        sprintf(buf, "ESP OK - %d sats", gps_info.satellites);
        ssd1306_display_text(&dev, 1, buf, strlen(buf), false);

        sprintf(buf, "Lat: %.6f", gps_info.latitude);
        ssd1306_display_text(&dev, 3, buf, strlen(buf), false);

        sprintf(buf, "Lon: %.6f", gps_info.longitude);
        ssd1306_display_text(&dev, 4, buf, strlen(buf), false);

        sprintf(buf, "Time: %.6s", gps_info.utc_time);
        ssd1306_display_text(&dev, 6, buf, strlen(buf), false);
    } else {
        ssd1306_display_text(&dev, 2, "NO FIX", 6, false);
        sprintf(buf, "Sats: %d", gps_info.satellites);
        ssd1306_display_text(&dev, 4, buf, strlen(buf), false);
        ssd1306_display_text(&dev, 6, "Waiting for signal...", 20, false);
    }

    ssd1306_show_buffer(&dev);
}

// ==================== MAIN ====================
void app_main(void)
{
    printf("Starting Heltec GPS Project...\n");

    // Enable Vext
    gpio_set_direction(VEXT_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(VEXT_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(150));

    // OLED reset
    gpio_set_direction(OLED_RESET_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(OLED_RESET_GPIO, 0);
    esp_rom_delay_us(20000);
    gpio_set_level(OLED_RESET_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    // I2C Setup
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
        .i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_NUM_17,
        .scl_io_num = GPIO_NUM_18,
    };

    i2c_master_bus_handle_t bus_handle = NULL;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x3C,
        .scl_speed_hz = 400000,
    };

    i2c_master_dev_handle_t dev_handle = NULL;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    dev._i2c_bus_handle = bus_handle;
    dev._i2c_dev_handle = dev_handle;

    ssd1306_init(&dev, 128, 64);

    // Splash
    ssd1306_clear_screen(&dev, false);
    ssd1306_display_text(&dev, 2, "GPS NEO-7M", 10, false);
    ssd1306_display_text(&dev, 4, "Waiting for GPS...", 17, false);
    ssd1306_show_buffer(&dev);
    vTaskDelay(pdMS_TO_TICKS(1500));

    // Start GPS task
    xTaskCreate(gps_uart_task, "gps_uart", 4096, NULL, 5, NULL);

    printf("GPS task started. Take module outdoors...\n");

    while (1) {
        display_gps();
        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}