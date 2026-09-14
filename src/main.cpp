#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"

#define LED_OUT GPIO_NUM_16
#define BUTTON_IN GPIO_NUM_18

static const char *TAG = "DELAY_EXAMPLE";

static bool IRAM_ATTR button_isr_handler(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx) {
    // bool led_state = gpio_get_level(LED_OUT);
    gpio_set_level(LED_OUT, 0);
    return false; // Return true to indicate that a higher priority task should be woken up
}   

extern "C" void app_main() {
    // init configuration for LED output
    gptimer_handle_t gptimer = NULL;
    gpio_config_t led_config = {
        .pin_bit_mask = (1ULL << LED_OUT),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1 MHz resolution
        .intr_priority = 1,
        .flags = {
            .intr_shared = 0,
            .allow_pd = 0
        }
    };

    gptimer_event_callbacks_t cbs = {
        .on_alarm = button_isr_handler, // Call the user callback function when the alarm event occurs
        };

    gptimer_register_event_callbacks(gptimer, &cbs, NULL);
    gptimer_new_timer(&timer_config, &gptimer);

    gptimer_alarm_config_t alarm_config = {
        .alarm_count = 5000, // 5 seconds
        .reload_count = 0,
        .flags = {
            .auto_reload_on_alarm = true
        },
    };

    gptimer_set_alarm_action(gptimer, &alarm_config);

 gptimer_enable(gptimer);
// Start the timer
gptimer_start(gptimer)  ;
    

    gpio_config_t button_config ={
        .pin_bit_mask = (1ULL << BUTTON_IN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE

    };

    // gpio config for LED output
    gpio_config(&led_config);
    // gpio config for button input
    gpio_config(&button_config);    
    gpio_set_level(LED_OUT, 1);
	while (1) {
        // bool button_state = gpio_get_level(BUTTON_IN);
        ESP_LOGI(TAG, "Задача виконується...");
        vTaskDelay(200 / portTICK_PERIOD_MS);
	}
}
