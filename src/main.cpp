#include <cstdio>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_flash.h"

static const char *TAG = "Main";

#define BUTTON_GPIO GPIO_NUM_4
#define POLL_INTERVAL_MS 10
#define DEBOUNCE_MS 50
#define DEBOUNCE_SAMPLES (DEBOUNCE_MS / POLL_INTERVAL_MS)

typedef enum {
    BTN_IDLE,
    BTN_PRESS_DETECTED,
    BTN_PRESSED,
    BTN_RELEASE_DETECTED
} button_state_t;

static void print_banner(void) {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);

    printf("\n========================================\n");
    printf("   ESP32-S3 Interrupts & Debounce\n");
    printf("   Task 4: Polling + state machine\n");
    printf("========================================\n");
    printf("Board: ESP32-S3-WROOM-1\n");
    printf("CPU Cores: %d @ %d MHz\n", chip_info.cores, CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
    printf("Flash: %lu MB\n", flash_size / (1024 * 1024));
    printf("Free Heap: %lu bytes\n", esp_get_free_heap_size());
    printf("========================================\n\n");
}

extern "C" void app_main(void) {
    // Wait for USB Serial JTAG to initialize
    vTaskDelay(pdMS_TO_TICKS(3000));

    print_banner();

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << BUTTON_GPIO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Button on GPIO %d (pull-down, active HIGH), polling every %d ms", BUTTON_GPIO, POLL_INTERVAL_MS);
    ESP_LOGI(TAG, "Debounce: %d ms (%d stable samples)", DEBOUNCE_MS, DEBOUNCE_SAMPLES);

    button_state_t state = BTN_IDLE;
    int stable_count = 0;
    uint32_t press_counter = 0;

    while (1) {
        int level = gpio_get_level(BUTTON_GPIO);

        switch (state) {
        case BTN_IDLE:
            if (level == 1) {
                state = BTN_PRESS_DETECTED;
                stable_count = 1;
            }
            break;

        case BTN_PRESS_DETECTED:
            if (level == 1) {
                stable_count++;
                if (stable_count >= DEBOUNCE_SAMPLES) {
                    state = BTN_PRESSED;
                    press_counter++;
                    ESP_LOGI(TAG, "Button pressed! Counter: %lu", press_counter);
                }
            } else {
                state = BTN_IDLE;
            }
            break;

        case BTN_PRESSED:
            if (level == 0) {
                state = BTN_RELEASE_DETECTED;
                stable_count = 1;
            }
            break;

        case BTN_RELEASE_DETECTED:
            if (level == 0) {
                stable_count++;
                if (stable_count >= DEBOUNCE_SAMPLES) {
                    state = BTN_IDLE;
                }
            } else {
                state = BTN_PRESSED;
            }
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));
    }
}
