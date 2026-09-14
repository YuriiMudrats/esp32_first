#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"
#include "Led.h"
#include "Adc.h"

#define LED_PIN GPIO_NUM_16
#define ADC_CHANNEL	ADC_CHANNEL_3 // ADC1_CH3: GPIO4// GPIO34 corresponds to ADC1 channel 6
#define bandwith 200;

Led_t led_config = {};
Adc_t adc_config = {};
 

extern "C" void app_main() {
    // init configuration for LED output

    Led_Init(&led_config, LED_PIN, OFF, 500, NULL);

    Adc_Init(&adc_config, ADC_CHANNEL, NULL);

    
	while (1) {
        // bool button_state = gpio_get_level(BUTTON_IN);
        Adc_GetValue(&adc_config);
        if(adc_config.voltage_mv / 1000.0f >= 2.6) {
            Led_Update(&led_config, OFF);
        } else {
             Led_Update(&led_config, ON);
        }   
        vTaskDelay(200 / portTICK_PERIOD_MS);
	}
}
