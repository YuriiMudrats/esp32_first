#include "uart.h"

#include <algorithm>
#include <cstring>

#include "driver/gpio.h"
#include "driver/uhci.h"
#include "driver/uart.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "hal/uart_ll.h"

#define UART_PORT UART_NUM_1
#define UART_BAUD_RATE 115200
#define UART_TX_PIN GPIO_NUM_17
#define UART_RX_PIN GPIO_NUM_18

static constexpr size_t UART_DMA_ALIGNMENT = 4U;
static constexpr size_t UART_DMA_BUFFER_SIZE = 1024U;
static constexpr size_t UART_DMA_MAX_TX_SIZE = 1024U;
static constexpr size_t UART_DMA_TX_QUEUE_DEPTH = 1U;

static const char *TAG = "uart";

static uhci_controller_handle_t s_uhci = NULL;
static SemaphoreHandle_t s_tx_mutex = NULL;
static SemaphoreHandle_t s_rx_mutex = NULL;
static SemaphoreHandle_t s_rx_done = NULL;
static uint8_t *s_tx_dma_buffer = NULL;
static uint8_t *s_rx_dma_buffer = NULL;
static uint8_t s_rx_frame[UART_DMA_BUFFER_SIZE];
static size_t s_rx_frame_size = 0U;
static bool s_rx_frame_overflow = false;
static bool s_rx_pending = false;
static bool s_uart_initialized = false;

static bool uart_dma_rx_callback(uhci_controller_handle_t controller,
                                 const uhci_rx_event_data_t *event,
                                 void *user_context)
{
    (void)controller;
    (void)user_context;

    const size_t available = sizeof(s_rx_frame) - s_rx_frame_size;
    const size_t copy_size = std::min(available, event->recv_size);
    if (copy_size > 0U) {
        memcpy(&s_rx_frame[s_rx_frame_size], event->data, copy_size);
        s_rx_frame_size += copy_size;
    }
    if (copy_size != event->recv_size) {
        s_rx_frame_overflow = true;
    }

    if (event->flags.totally_received) {
        BaseType_t task_woken = pdFALSE;
        xSemaphoreGiveFromISR(s_rx_done, &task_woken);
        return task_woken == pdTRUE;
    }
    return false;
}

static esp_err_t uart_dma_transmit_chunk(const uint8_t *data, size_t size)
{
    memcpy(s_tx_dma_buffer, data, size);
    ESP_RETURN_ON_ERROR(uhci_transmit(s_uhci, s_tx_dma_buffer, size),
                        TAG, "DMA transmit submit failed");
    return uhci_wait_all_tx_transaction_done(s_uhci, -1);
}

static void uart_write_fifo_tail(const uint8_t *data, size_t size)
{
    uart_dev_t *const hw = UART_LL_GET_HW(UART_PORT);
    size_t sent = 0U;
    while (sent < size) {
        const uint32_t writable = uart_ll_get_txfifo_len(hw);
        if (writable == 0U) {
            vTaskDelay(1);
            continue;
        }
        const size_t count = std::min(static_cast<size_t>(writable), size - sent);
        uart_ll_write_txfifo(hw, &data[sent], static_cast<uint32_t>(count));
        sent += count;
    }
}

esp_err_t uart_init(void)
{
    if (s_uart_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    s_tx_mutex = xSemaphoreCreateMutex();
    s_rx_mutex = xSemaphoreCreateMutex();
    s_rx_done = xSemaphoreCreateBinary();
    s_tx_dma_buffer = static_cast<uint8_t *>(
        heap_caps_malloc(UART_DMA_BUFFER_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
    s_rx_dma_buffer = static_cast<uint8_t *>(
        heap_caps_malloc(UART_DMA_BUFFER_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
    if (s_tx_mutex == NULL || s_rx_mutex == NULL || s_rx_done == NULL ||
        s_tx_dma_buffer == NULL || s_rx_dma_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate UART DMA resources");
        uart_deinit();
        return ESP_ERR_NO_MEM;
    }

    uart_config_t uart_config = {};
    uart_config.baud_rate = UART_BAUD_RATE;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_DEFAULT;

    esp_err_t err = uart_param_config(UART_PORT, &uart_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART configuration failed: %s", esp_err_to_name(err));
        uart_deinit();
        return err;
    }

    err = uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART pin configuration failed: %s", esp_err_to_name(err));
        uart_deinit();
        return err;
    }

    const uhci_controller_config_t uhci_config = {
        .uart_port = UART_PORT,
        .tx_trans_queue_depth = UART_DMA_TX_QUEUE_DEPTH,
        .max_transmit_size = UART_DMA_MAX_TX_SIZE,
        .max_receive_internal_mem = UART_DMA_BUFFER_SIZE,
        .dma_burst_size = 0U,
        .max_packet_receive = 0U,
        .rx_eof_flags = {
            .rx_brk_eof = 0U,
            .idle_eof = 1U,
            .length_eof = 0U,
        },
    };
    err = uhci_new_controller(&uhci_config, &s_uhci);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UHCI DMA initialization failed: %s", esp_err_to_name(err));
        uart_deinit();
        return err;
    }

    const uhci_event_callbacks_t callbacks = {
        .on_rx_trans_event = uart_dma_rx_callback,
        .on_tx_trans_done = NULL,
    };
    err = uhci_register_event_callbacks(s_uhci, &callbacks, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UHCI callback registration failed: %s", esp_err_to_name(err));
        uart_deinit();
        return err;
    }

    s_uart_initialized = true;
    ESP_LOGI(TAG, "UART1 ready with UHCI DMA: %d baud, 8N1, TX GPIO%d, RX GPIO%d",
             UART_BAUD_RATE, UART_TX_PIN, UART_RX_PIN);
    return ESP_OK;
}

esp_err_t uart_deinit(void)
{
    esp_err_t result = ESP_OK;
    if (s_uhci != NULL) {
        result = uhci_wait_all_tx_transaction_done(s_uhci, -1);
        const esp_err_t err = uhci_del_controller(s_uhci);
        if (result == ESP_OK) {
            result = err;
        }
        s_uhci = NULL;
    }
    if (s_tx_dma_buffer != NULL) {
        heap_caps_free(s_tx_dma_buffer);
        s_tx_dma_buffer = NULL;
    }
    if (s_rx_dma_buffer != NULL) {
        heap_caps_free(s_rx_dma_buffer);
        s_rx_dma_buffer = NULL;
    }
    if (s_tx_mutex != NULL) {
        vSemaphoreDelete(s_tx_mutex);
        s_tx_mutex = NULL;
    }
    if (s_rx_mutex != NULL) {
        vSemaphoreDelete(s_rx_mutex);
        s_rx_mutex = NULL;
    }
    if (s_rx_done != NULL) {
        vSemaphoreDelete(s_rx_done);
        s_rx_done = NULL;
    }
    s_rx_frame_size = 0U;
    s_rx_frame_overflow = false;
    s_rx_pending = false;
    s_uart_initialized = false;
    return result;
}

esp_err_t uart_receive_frame(uint8_t *data, size_t capacity, size_t *received_size,
                             uint32_t timeout_ms)
{
    if (data == NULL || capacity == 0U || received_size == NULL) {
        ESP_LOGE(TAG, "Invalid UART receive arguments");
        return ESP_ERR_INVALID_ARG;
    }
    *received_size = 0U;
    if (!s_uart_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_rx_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    if (!s_rx_pending && s_rx_frame_size == 0U) {
        s_rx_frame_overflow = false;
        if (xSemaphoreTake(s_rx_done, 0U) == pdTRUE) {
            ESP_LOGE(TAG, "Unexpected stale UART DMA completion");
            xSemaphoreGive(s_rx_mutex);
            return ESP_ERR_INVALID_STATE;
        }
        const esp_err_t err = uhci_receive(s_uhci, s_rx_dma_buffer,
                                           UART_DMA_BUFFER_SIZE);
        if (err == ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG, "UART DMA receive is already active; waiting for its completion");
            s_rx_pending = true;
        } else if (err != ESP_OK) {
            ESP_LOGE(TAG, "DMA receive start failed: %s", esp_err_to_name(err));
            xSemaphoreGive(s_rx_mutex);
            return err;
        } else {
            s_rx_pending = true;
        }
    }

    if (s_rx_pending && xSemaphoreTake(s_rx_done, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) {
        xSemaphoreGive(s_rx_mutex);
        return ESP_ERR_TIMEOUT;
    }

    s_rx_pending = false;
    if (s_rx_frame_overflow) {
        s_rx_frame_size = 0U;
        s_rx_frame_overflow = false;
        xSemaphoreGive(s_rx_mutex);
        ESP_LOGE(TAG, "UART DMA receive frame exceeds %u bytes",
                 static_cast<unsigned int>(UART_DMA_BUFFER_SIZE));
        return ESP_ERR_INVALID_SIZE;
    }
    if (s_rx_frame_size == 0U) {
        xSemaphoreGive(s_rx_mutex);
        return ESP_ERR_TIMEOUT;
    }

    const size_t bytes_to_copy = std::min(capacity, s_rx_frame_size);
    memcpy(data, s_rx_frame, bytes_to_copy);
    s_rx_frame_size -= bytes_to_copy;
    *received_size = bytes_to_copy;
    if (s_rx_frame_size > 0U) {
        memmove(s_rx_frame, &s_rx_frame[bytes_to_copy], s_rx_frame_size);
    }
    xSemaphoreGive(s_rx_mutex);
    return ESP_OK;
}

esp_err_t uart_receive(uint8_t *data, size_t size, uint32_t timeout_ms)
{
    size_t received_size = 0U;
    return uart_receive_frame(data, size, &received_size, timeout_ms);
}

esp_err_t uart_transmit(const uint8_t *data, size_t size)
{
    if (data == NULL || size == 0U) {
        ESP_LOGE(TAG, "Invalid UART transmit arguments");
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_uart_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_tx_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    const uint8_t *cursor = data;
    size_t remaining = size;
    esp_err_t err = ESP_OK;
    while (remaining >= UART_DMA_ALIGNMENT) {
        const size_t chunk_size = std::min(remaining, UART_DMA_MAX_TX_SIZE);
        const size_t aligned_chunk_size =
            chunk_size - (chunk_size % UART_DMA_ALIGNMENT);
        err = uart_dma_transmit_chunk(cursor, aligned_chunk_size);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "UART DMA transmit failed: %s", esp_err_to_name(err));
            break;
        }
        cursor += aligned_chunk_size;
        remaining -= aligned_chunk_size;
    }

    if (err == ESP_OK && remaining > 0U) {
        uart_write_fifo_tail(cursor, remaining);
    }

    if (err == ESP_OK) {
        uart_dev_t *const hw = UART_LL_GET_HW(UART_PORT);
        while (!uart_ll_is_tx_idle(hw)) {
            vTaskDelay(1);
        }
    }
    xSemaphoreGive(s_tx_mutex);
    return err;
}
