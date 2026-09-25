#include <stdio.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pwm.h"
#include "Adc.h"
#include "servo.h"

#define ADC_CHANNEL ADC_CHANNEL_3 // ADC1_CH3: GPIO4// GPIO34 corresponds to ADC1 channel 6
#define SERVO_GPIO GPIO_NUM_15
#define SERVO_INITIAL_ANGLE 90
#define SERVO_MOVE_STEP 45
#define ADC_MAX_RAW 4095

Adc_t adc_config = {};
pwm_t servo_pwm = {};

static const char *TAG = "main";

static uint16_t map_adc_to_angle(int adc_raw)
{
    if (adc_raw <= 0) {
        return SERVO_MIN_ANGLE;
    }
    if (adc_raw >= ADC_MAX_RAW) {
        return SERVO_MAX_ANGLE;
    }

    return static_cast<uint16_t>(
        (static_cast<uint32_t>(adc_raw) * SERVO_MAX_ANGLE) / ADC_MAX_RAW);
}


extern "C" void app_main()
{
    // init configuration for LED output
   esp_err_t last_error = ESP_OK;
   int servo_init_angle = 0;

    // 1. Конфігурація PWM для SG90
    pwm_t servo_pwm = {};
    pwm_config_t servo_cfg = {
        .gpio = SERVO_GPIO,
        .channel = LEDC_CHANNEL_0,
        .timer = LEDC_TIMER_0,
        .frequency_hz = SERVO_FREQUENCY_HZ,
        .resolution = LEDC_TIMER_14_BIT,
        .duty = 0,
        .inverted = false
    };

    Adc_Init(&adc_config, ADC_CHANNEL, NULL);
    esp_err_t ret = pwm_init(&servo_pwm, &servo_cfg);
    if (ret != ESP_OK) {
        last_error = ret;
        ESP_LOGE(TAG, "pwm_init failed: %s", esp_err_to_name(ret));
    }


      // 2. Ініціалізація сервомотора
    servo_t servo = {};
    ret = servo_init(&servo, &servo_pwm);
    if (ret != ESP_OK) {
        last_error = ret;
        ESP_LOGE(TAG, "servo_init failed: %s", esp_err_to_name(ret));
        pwm_deinit(&servo_pwm);
    } else {
        // Початкове положення сервомотора.
        ret = servo_set_angle(&servo, SERVO_INITIAL_ANGLE);
        if (ret != ESP_OK) {
            last_error = ret;
            ESP_LOGE(TAG, "servo_set_angle failed: %s", esp_err_to_name(ret));
        }
    }

    while (1) {
        if (last_error != ESP_OK) {
            printf("Last init error: %s\n", esp_err_to_name(last_error));
        } else {
            Adc_GetValue(&adc_config);
            uint16_t angle = map_adc_to_angle(adc_config.adc_raw);
            ret = servo_set_angle(&servo, angle);
            if (ret != ESP_OK) {
                last_error = ret;
                ESP_LOGE(TAG, "servo_set_angle failed: %s", esp_err_to_name(ret));
            } else {
                printf("ADC raw: %d, servo angle: %u degrees\n",
                       adc_config.adc_raw,
                       angle);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}