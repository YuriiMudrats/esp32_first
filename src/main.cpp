#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"
#include "Adc.h"
#include "pwm.h"

#define RELAY_PIN GPIO_NUM_16
#define ADC_CHANNEL ADC_CHANNEL_3 // ADC1_CH3: GPIO4// GPIO34 corresponds to ADC1 channel 6
#define MOTOR_OUT GPIO_NUM_17

Adc_t adc_config = {};
Pwm_t pwm_config;


extern "C" void app_main()
{
    // init configuration for LED output
    gpio_reset_pin(RELAY_PIN);
    gpio_set_direction(RELAY_PIN, GPIO_MODE_OUTPUT);

    int pwm_init_result = Pwm_Init(&pwm_config, MOTOR_OUT);
    if (pwm_init_result != 0) {
        printf("Failed to initialize PWM, err = %d\n", pwm_init_result);
    }

    Adc_Init(&adc_config, ADC_CHANNEL, NULL);


    while (1)
    {
        Adc_GetValue(&adc_config);
        uint32_t pwm_duty = adc_config.adc_raw < 0
            ? 0U
            : static_cast<uint32_t>(adc_config.adc_raw);
        if (pwm_duty > pwm_config.max_duty) {
            pwm_duty = pwm_config.max_duty;
        }
        if (pwm_init_result == 0) {
            Pwm_UpdateDuty(&pwm_config, pwm_duty);
        }
        printf("PWM GPIO %d: ADC %d mV, duty %lu/4095\n",
               MOTOR_OUT,
               adc_config.voltage_mv,
               static_cast<unsigned long>(pwm_duty));
        float current_v = adc_config.voltage_mv / 1000.0f;
        if(current_v>0){
            printf("RELAY: %s\n", "ON");
            gpio_set_level(RELAY_PIN, 1);
        } else {
            printf("RELAY: %s\n", "OFF");
            gpio_set_level(RELAY_PIN, 0);
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}
