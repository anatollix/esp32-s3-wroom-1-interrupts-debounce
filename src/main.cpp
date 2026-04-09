#include <cstdio>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_flash.h"

static const char *TAG = "Main";

static void print_banner(void) {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);

    printf("\n========================================\n");
    printf("   ESP32-S3 Interrupts & Debounce\n");
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

    ESP_LOGI(TAG, "Ready. See task branches for implementations.");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
