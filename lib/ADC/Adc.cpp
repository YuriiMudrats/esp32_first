#include "Adc.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"



int Adc_Init(Adc_t *ctx, adc_channel_t pin, AdcCb_t pressedCb) {
    if (ctx == NULL ) {
        return -1;
    }

    esp_err_t err = ESP_OK;

    ctx->pressedCb = pressedCb;
    ctx->pin = pin;
    ctx->arg = NULL;

    // Конфігурація GPIO для світлодіода
    ctx->adc_config = {};
    ctx->adc_config.unit_id = ADC_UNIT_1;
    ctx->adc_config.clk_src = ADC_RTC_CLK_SRC_DEFAULT;
    ctx->adc_config.ulp_mode = ADC_ULP_MODE_DISABLE;
    ctx->adc_handle = NULL;

    err = adc_oneshot_new_unit(&ctx->adc_config, &ctx->adc_handle);
    if (err != ESP_OK) {
        printf("Failed to initialize ADC, err = %d\n", err);
        return -1;
    } 


    adc_oneshot_chan_cfg_t adc_channel_config = {};
    adc_channel_config.bitwidth = ADC_BITWIDTH_DEFAULT;
    adc_channel_config.atten = ADC_ATTEN_DB_12;
    err = adc_oneshot_config_channel(ctx->adc_handle, ctx->pin, &adc_channel_config);
    if (err != ESP_OK) {
        printf("Failed to configure ADC channel, err = %d\n", err);
        return -1;
    }

    adc_cali_handle_t calibration_handle = nullptr;
    adc_cali_curve_fitting_config_t calibration_config = {};
    calibration_config.unit_id = ADC_UNIT_1;
    calibration_config.chan = ctx->pin;
    calibration_config.atten = ADC_ATTEN_DB_12;
    calibration_config.bitwidth = ADC_BITWIDTH_DEFAULT;
    bool calibration_enabled =
        adc_cali_create_scheme_curve_fitting(&calibration_config, &calibration_handle) == ESP_OK;
    ctx->calibration_handle = calibration_handle;       
    ctx->calibration_enabled = calibration_enabled;


    return calibration_enabled? 1 : 0;
}


int Adc_GetValue(Adc_t *ctx) {

    if (ctx == NULL) {
        return -1;
    }

     esp_err_t err = ESP_OK;

     err = adc_oneshot_read(ctx->adc_handle, ctx->pin, &ctx->adc_raw);
        if (err == ESP_OK) {
            if (ctx->calibration_enabled) {
                adc_cali_raw_to_voltage(ctx->calibration_handle, ctx->adc_raw, &ctx->voltage_mv);
            } else {
                printf("ADC %d raw: %d (calibration unavailable)\n", ctx->pin, ctx->adc_raw);
            }
        } else {
            printf("Failed to read ADC, err = %d\n", err);
        }

    return ctx->calibration_enabled;
}