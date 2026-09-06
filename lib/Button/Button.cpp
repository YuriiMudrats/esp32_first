#include "Button.h"

int Button_Init(Button_FSM_t *ctx, uint8_t buttonPin, ButtonState state, int debounceTime, ButtonCallback_t pressedCb) {
    if (ctx == NULL) {
        Serial.println("Error: FSM pointer is NULL");
        return -1;
    }

    ctx->pin = buttonPin;
    ctx->state = state;
    ctx->pressedCb = pressedCb;
    ctx->debounceTime = debounceTime;
    ctx->currentTime = millis();
    ctx->arg = NULL;

    pinMode(buttonPin, INPUT_PULLUP);
    return 0;
}

int Button_FSM_Update(Button_FSM_t *ctx) {
    bool pressed;

    if (ctx == NULL) {
        Serial.println("Error: FSM pointer is NULL");
        return -1;
    }

    pressed = (digitalRead(ctx->pin) == LOW);

    if (pressed && ctx->state != PRESSED) {
        if (millis() - ctx->currentTime > ctx->debounceTime) {
        ctx->currentTime = millis();
        ctx->state = PRESSED;
        if (ctx->pressedCb != NULL) {
            ctx->pressedCb(ctx->arg);
        }
        }
    } else if (!pressed && ctx->state != IDLE) {
        ctx->state = IDLE;
    }

    return 0;
}

bool Button_If_Pressed(Button_FSM_t *ctx) {
    if (ctx == NULL) {
        Serial.println("Error: FSM pointer or state pointer is NULL");
        return false;
    }

    return (ctx->state == PRESSED);
}