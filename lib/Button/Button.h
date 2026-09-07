#ifndef BUTTON_FSM_H
#define BUTTON_FSM_H

#include <Arduino.h>

typedef enum {
    IDLE,
    PRESSED,
    LONG_PRESS,
    RELEASED
} ButtonState;

typedef void (*ButtonCallback_t)(void* arg);

typedef struct {
    uint8_t pin;
    ButtonState state;
    ButtonCallback_t pressedCb;
    ButtonCallback_t longPressCb;
    
    unsigned long longDelayTime;
    unsigned long longPressTime;
    int debounce;
    void* arg;
} Button_FSM_t;

int Button_Init(Button_FSM_t *ctx, uint8_t buttonPin, ButtonState state,  ButtonCallback_t pressedCb, ButtonCallback_t longPressCb);
int Button_Update(Button_FSM_t *ctx);
ButtonState Button_If_Pressed(Button_FSM_t *ctx);


#endif