#ifndef LED_FSM_H
#define LED_FSM_H

#include <Arduino.h>

typedef enum {
    ON,
    OFF,
    BLINK
} LedState;

typedef void (*LedCb_t)(void* arg);
typedef bool (*LedChangeTrigger_t)(void* arg);

typedef struct {
    uint8_t pin;
    LedState state;
    uint32_t debounce;
    unsigned long lastTime;
    LedCb_t pressedCb;
    LedChangeTrigger_t triggerCb;
    void* arg;
} Led_t;

int Led_Init(Led_t *ctx, uint8_t pin, LedState state, uint32_t debounce, LedCb_t pressedCb);
int Led_Update(Led_t *ctx, bool isActive);
int Led_Reset(Led_t *ctx);


#endif