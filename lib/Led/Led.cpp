#include "Led.h"

int Led_Init(Led_t *ctx, uint8_t pin, LedState state, uint32_t debounce, LedCb_t pressedCb) {
    if (ctx == NULL) {
        return -1;
    }

    ctx->pin = pin;
    ctx->state = state;
    ctx->debounce = debounce;
    ctx->lastTime = millis();
    ctx->pressedCb = pressedCb;
    ctx->arg = NULL;

    pinMode(pin, OUTPUT);
    digitalWrite(pin, (state == ON) ? HIGH : LOW);

    return 0;
}

static void applyPinState(uint8_t pin, bool activeLevel) {
    digitalWrite(pin, activeLevel ? HIGH : LOW);
}

int Led_Update(Led_t *ctx, LedState mode) {
    unsigned long now = millis();

    if (ctx == NULL) {
        return -1;
    }

    switch (mode) {
        case ON:
            ctx->state = ON;
            applyPinState(ctx->pin, true);
            break;
        case OFF:
            ctx->state = OFF;
            applyPinState(ctx->pin, false);
            break;
        case BLINK:
            ctx->state = BLINK;
            if (now - ctx->lastTime >= ctx->debounce) {
                ctx->lastTime = now;
                bool newState = digitalRead(ctx->pin) == LOW;
                applyPinState(ctx->pin, newState);
            }
            break;
        default:
            return -1; // Invalid mode
    }

    return 0;
}