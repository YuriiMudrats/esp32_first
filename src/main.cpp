#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "uart.h"

#define UART_RX_DMA_BUFFER_SIZE 1024U
#define UART_RX_TIMEOUT_MS 1000U

static const char *TAG = "UART_TERMINAL_RX";

static void uart_receive_task(void *arg)
{
    (void)arg;
    uint8_t *const rx_buffer = static_cast<uint8_t *>(
        heap_caps_malloc(UART_RX_DMA_BUFFER_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
    if (rx_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate UART receive DMA buffer");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "UART DMA terminal receiver ready; waiting for idle-delimited input");
    while (true) {
        size_t received_size = 0U;
        const esp_err_t err = uart_receive_frame(rx_buffer, UART_RX_DMA_BUFFER_SIZE,
                                                 &received_size, UART_RX_TIMEOUT_MS);
        if (err == ESP_ERR_TIMEOUT) {
            continue;
        }
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "UART DMA receive failed: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        ESP_LOGI(TAG, "Stored %u UART bytes in the DMA-capable receive buffer",
                 static_cast<unsigned int>(received_size));
        ESP_LOG_BUFFER_HEXDUMP(TAG, rx_buffer, received_size, ESP_LOG_INFO);
    }
}

extern "C" void app_main() {
    ESP_ERROR_CHECK(uart_init());

    if (xTaskCreate(uart_receive_task, "uart_rx", 4096, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create UART DMA receive task");
        ESP_ERROR_CHECK(uart_deinit());
        return;
    }
    ESP_LOGI(TAG, "Send terminal data to UART1 RX GPIO18 at 115200 baud, 8N1");
}
