#ifndef SWITCH_FSM_H
#define SWITCH_FSM_H

#include <Arduino.h>

typedef enum {
    ON,
    OFF,
} SwitchState;

typedef void (*SwitchCb_t)(void* arg);
typedef bool (*SwitchChangeTrigger_t)(void* arg);

typedef struct {
    uint8_t pin;
    SwitchState state;
    uint32_t debounce;
    unsigned long lastTime;
    SwitchCb_t pressedCb;
    SwitchChangeTrigger_t triggerCb;
    void* arg;
} Switch_t;

int Switch_Init(Switch_t *ctx, uint8_t pin, SwitchState state, uint32_t debounce, SwitchCb_t pressedCb);
int Switch_Update(Switch_t *ctx, bool isActive);
int Switch_Reset(Switch_t *ctx);


#endif  