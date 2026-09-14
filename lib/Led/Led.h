#ifndef LED_FSM_H
#define LED_FSM_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"



typedef enum {
    OFF,
    ON,
    BLINK
} LedState;

typedef void (*LedCb_t)(void* arg);
typedef bool (*LedChangeTrigger_t)(void* arg);

typedef struct {
    gpio_config_t  led_config;
    gptimer_handle_t timer = NULL;
    gptimer_config_t timer_config;
    bool timer_enabled = false;
    LedState state;
    uint32_t debounce;
    unsigned long lastTime;
    LedCb_t pressedCb;
    LedChangeTrigger_t triggerCb;
    gpio_num_t pin;
    void* arg;
} Led_t;


int Led_Init(Led_t *ctx, gpio_num_t pin, LedState state, uint32_t debounce, LedCb_t pressedCb);
int Led_Update(Led_t *ctx, LedState mode);
int Led_Reset(Led_t *ctx);


#endif