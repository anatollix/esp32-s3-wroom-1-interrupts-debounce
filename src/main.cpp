#include <cstdio>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_flash.h"

static const char *TAG = "Main";

#define BUTTON_GPIO GPIO_NUM_4
#define DEBOUNCE_MS 50

static TaskHandle_t button_task_handle = NULL;
static uint32_t press_counter = 0;

static void IRAM_ATTR button_isr_handler(void *arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(button_task_handle, 0, eNoAction, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void button_task(void *arg) {
    while (1) {
        xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);

        // Wait for bounce to settle
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));

        // Only count if button is still pressed (HIGH for pull-down button)
        if (gpio_get_level(BUTTON_GPIO) == 1) {
            press_counter++;
            ESP_LOGI(TAG, "Button pressed! Counter: %lu", press_counter);

            // Wait for release, then debounce the release
            while (gpio_get_level(BUTTON_GPIO) == 1) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
        }

        // Clear any notifications accumulated during processing
        xTaskNotifyStateClear(NULL);
    }
}

static void print_banner(void) {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);

    printf("\n========================================\n");
    printf("   ESP32-S3 Interrupts & Debounce\n");
    printf("   Task 3: State-based debounce\n");
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

    xTaskCreate(button_task, "button_task", 2048, NULL, 10, &button_task_handle);

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << BUTTON_GPIO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL);

    ESP_LOGI(TAG, "Button on GPIO %d (pull-down, active HIGH), interrupt on RISING edge", BUTTON_GPIO);
    ESP_LOGI(TAG, "ISR signals task, task checks if button is still pressed");
}
