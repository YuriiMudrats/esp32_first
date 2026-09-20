#ifndef ADC_FSM_H
#define ADC_FSM_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"



typedef void (*AdcCb_t)(void* arg);

typedef struct {
    adc_oneshot_unit_init_cfg_t  adc_config;
    adc_oneshot_unit_handle_t adc_handle = NULL;
    AdcCb_t pressedCb;
    int adc_raw = 0;
    int voltage_mv = 0;
    adc_channel_t pin;
    bool calibration_enabled = false;
    adc_cali_handle_t calibration_handle = nullptr;
    void* arg;
} Adc_t;


int Adc_Init(Adc_t *ctx, adc_channel_t pin, AdcCb_t pressedCb);
int Adc_GetValue(Adc_t *ctx);


#endif