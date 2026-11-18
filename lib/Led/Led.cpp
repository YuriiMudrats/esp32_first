#include "Led.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"

volatile uint8_t led_state = 0;

// 1. Interrupt Service Routine (Callback)
static bool IRAM_ATTR timer_on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *ctx) {
    // Keep code inside ISR short and fast!
    led_state = !led_state;
    gpio_set_level(((Led_t *)ctx)->pin, led_state);
    
    return false; 
}

int Led_Init(Led_t *ctx, gpio_num_t pin, LedState state, uint32_t debounce, LedCb_t pressedCb) {
    if (ctx == NULL ) {
        return -1;
    }

    ctx->state = state;
    ctx->debounce = debounce;
    ctx->pressedCb = pressedCb;
    ctx->pin = pin;
    ctx->arg = NULL;
    ctx->timer_enabled = false;

    // Конфігурація GPIO для світлодіода
    ctx->led_config.pin_bit_mask = (1ULL << pin);
    ctx->led_config.mode = GPIO_MODE_OUTPUT;
    ctx->led_config.pull_up_en = GPIO_PULLUP_DISABLE;
    ctx->led_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    ctx->led_config.intr_type = GPIO_INTR_DISABLE;  


    // Конфігурація GPIO
    gpio_config(&ctx->led_config);

    // Встановлення початкового стану
    gpio_set_level(ctx->pin, 0);


    gptimer_handle_t gptimer = NULL;
    gptimer_config_t timer_config = {};
    timer_config.clk_src = GPTIMER_CLK_SRC_DEFAULT; // Typically APB clock (80MHz)
    timer_config.direction = GPTIMER_COUNT_UP;
    timer_config.resolution_hz = 1000000;           // 1MHz resolution = 1 tick equals 1 microsecond
    timer_config.intr_priority = 1;
    timer_config.flags.intr_shared = 0;
    timer_config.flags.allow_pd = 0;
    // create a new timer
    gptimer_new_timer(&timer_config, &gptimer);
    ctx->timer_config = timer_config; 
    ctx->timer = gptimer;
    
     gptimer_event_callbacks_t cbs = {};
    cbs.on_alarm = timer_on_alarm_cb;

    gptimer_register_event_callbacks(gptimer, &cbs, ctx);

    gptimer_alarm_config_t alarm_config = {};
    alarm_config.alarm_count = static_cast<uint64_t>(debounce) * 1000ULL;
    alarm_config.reload_count = 0; // Reset to 0 on reload
    alarm_config.flags.auto_reload_on_alarm = true; // Automatically restart timer after alarm
    gptimer_set_alarm_action(gptimer, &alarm_config);


    return 0;
}

static void applyPinState(gpio_num_t pin, bool activeLevel) {
    gpio_set_level(pin, activeLevel ? 1 : 0);
}

int Led_Update(Led_t *ctx, LedState mode) {

    if (ctx == NULL) {
        return -1;
    }

    switch (mode) {
        case ON:
            ctx->state = ON;
            applyPinState(ctx->pin, true);
            if (ctx->timer && ctx->timer_enabled) {
                gptimer_stop(ctx->timer);
                gptimer_disable(ctx->timer);
                ctx->timer_enabled = false;
            }
            break;
        case OFF:
            ctx->state = OFF;
            applyPinState(ctx->pin, false);
            if (ctx->timer && ctx->timer_enabled) {
                gptimer_stop(ctx->timer);
                gptimer_disable(ctx->timer);
                ctx->timer_enabled = false;
            }
            break;
        case BLINK:
            ctx->state = BLINK;
            if (ctx->timer && !ctx->timer_enabled) {
                gptimer_enable(ctx->timer);
                ctx->timer_enabled = true;
                gptimer_start(ctx->timer);
            }
            break;
        default:
            return -1; // Invalid mode
    }

    return 0;
}