#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "uart.h"

static const char *TAG = "app";

#define LED_GPIO_NUM GPIO_NUM_15
#define SWITCH_GPIO_NUM GPIO_NUM_16

extern "C" void app_main()
{
    const esp_err_t init_err = uart_init();
    gpio_reset_pin(LED_GPIO_NUM);
    gpio_set_direction(LED_GPIO_NUM, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO_NUM, 0);

    gpio_reset_pin(SWITCH_GPIO_NUM);
    gpio_set_direction(SWITCH_GPIO_NUM, GPIO_MODE_INPUT);
    gpio_set_pull_mode(SWITCH_GPIO_NUM, GPIO_PULLUP_ONLY);

    if (init_err != ESP_OK)
    {
        ESP_LOGE(TAG, "UART initialization failed: %s", esp_err_to_name(init_err));
        return;
    }

    ESP_LOGI(TAG, "UART echo application started");

    uint8_t received_byte;
    uint8_t transmit_byte;
    while (1)
    {
        const esp_err_t receive_err = uart_receive(&received_byte, 1, 0);
        if (receive_err == ESP_OK)
        {
            ESP_LOGI(TAG, "received byte: 0x%02X - '%c'", received_byte, received_byte);
            if (received_byte == '1')
            {
                gpio_set_level(LED_GPIO_NUM, 1);
                ESP_LOGI(TAG, "LED is turned on");
            }
            else if (received_byte == '0')
            {
                gpio_set_level(LED_GPIO_NUM, 0);
                ESP_LOGI(TAG, "LED is turned off");
            }

            if (gpio_get_level(SWITCH_GPIO_NUM) == 0)
            {
                transmit_byte = '1';
                ESP_LOGI(TAG, "switch is pressed");
            }
            else
            {
                transmit_byte = '0';
                ESP_LOGI(TAG, "switch is not pressed");
            }

            const esp_err_t transmit_err = uart_transmit(&transmit_byte, 1);
            if (transmit_err != ESP_OK)
            {
                ESP_LOGE(TAG, "UART transmit error: %s", esp_err_to_name(transmit_err));
            }
        }
        else if (receive_err != ESP_ERR_TIMEOUT)
        {
            ESP_LOGE(TAG, "UART receive error: %s", esp_err_to_name(receive_err));
        }

        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}
