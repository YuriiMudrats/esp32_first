#ifndef ESP32_FIRST_SEMAPHORE_H
#define ESP32_FIRST_SEMAPHORE_H

#include <Arduino.h>
#include "Button.h"
#include "Led.h"

typedef enum {
    GREEN,
    GREEN_BLINK,
    YELLOW,
    RED,
    RED_YELLOW,
    NONE
} SemaphoreState;

typedef void (*SemaphoreCb_t)(void* arg);
typedef bool (*SemaphoreChangeTrigger_t)(void* arg);

typedef struct {
    SemaphoreState state;
    Led_t led_green;
    Led_t led_yellow;
    Led_t led_red;
    unsigned long lastTime;
    SemaphoreCb_t pressedCb;
    SemaphoreChangeTrigger_t triggerCb;
    void* arg;
} Semaphore_t;

int Semaphore_Init(Semaphore_t *ctx);
int Semaphore_Update(Semaphore_t *ctx, ButtonState buttonState);
int Semaphore_Reset(Semaphore_t *ctx);


#endif