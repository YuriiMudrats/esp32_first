#ifndef BUTTON_FSM_H
#define BUTTON_FSM_H

#include <Arduino.h>

typedef enum {
    IDLE,
    PRESSED,
    LONG_PRESS
} ButtonState;

typedef void (*ButtonCallback_t)(void* arg);

typedef struct {
    uint8_t pin;
    ButtonState state;
    bool isPressed;
    ButtonCallback_t pressedCb;
    void* arg;
} Button_FSM_t;

int Button_Init(Button_FSM_t *ctx, uint8_t buttonPin, ButtonState state,  ButtonCallback_t pressedCb);
int Button_FSM_Update(Button_FSM_t *ctx);
bool Button_If_Pressed(Button_FSM_t *ctx);


#endif