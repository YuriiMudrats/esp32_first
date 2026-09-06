#include "Switch.h"

int Switch_Init(Switch_t *ctx, uint8_t pin, SwitchState state, uint32_t debounce, SwitchCb_t pressedCb) {
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

int Switch_Update(Switch_t *ctx, bool isActive ) {

    if (ctx == NULL) {
        return -1;
    }

    if (!isActive) {
        ctx->state = OFF;
        applyPinState(ctx->pin, false);
        return 0;
    } else {
        bool stateChanged = (ctx->state != ON);
        ctx->state = ON;
        applyPinState(ctx->pin, true);
        if (stateChanged && ctx->pressedCb != NULL) {
            ctx->pressedCb(ctx->arg);
        }
    }

    return 0;
}