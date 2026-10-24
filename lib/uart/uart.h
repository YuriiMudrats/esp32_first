#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t uart_init(void);
esp_err_t uart_deinit(void);
/* Receives one idle-delimited frame using UHCI DMA; frames are limited to 1024 bytes. */
esp_err_t uart_receive(uint8_t *data, size_t size, uint32_t timeout_ms);
esp_err_t uart_receive_frame(uint8_t *data, size_t capacity, size_t *received_size,
                             uint32_t timeout_ms);
/* Transmits through UHCI DMA, using the UART FIFO for any trailing 1-3 bytes. */
esp_err_t uart_transmit(const uint8_t *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif
