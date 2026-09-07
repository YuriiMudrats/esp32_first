#include "Button.h"


int Button_Init(Button_FSM_t *ctx, uint8_t buttonPin, ButtonState state, ButtonCallback_t pressedCb, ButtonCallback_t longPressCb) {
    if (ctx == NULL) {
        Serial.println("Error: FSM pointer is NULL");
        return -1;
    }

    ctx->pin = buttonPin;
    ctx->state = state;
    ctx->pressedCb = pressedCb;
    ctx->longPressCb = longPressCb;
    ctx->arg = NULL;
    ctx->longDelayTime = 3000;
    ctx->longPressTime = 0;
    ctx->debounce = 50; // Set a default debounce time of 50 milliseconds

    pinMode(buttonPin, INPUT_PULLUP);
    return 0;
}

int Button_Update(Button_FSM_t *ctx) {
    bool pressed;

    if (ctx == NULL) {
        Serial.println("Error: FSM pointer is NULL");
        return -1;
    }

    pressed = (digitalRead(ctx->pin) == LOW);

    if (pressed && ctx->state != PRESSED && ctx->state != LONG_PRESS) {
        if (millis() - ctx->longPressTime < ctx->debounce) {
            return 0; // Ignore if within debounce time
        }
        ctx->state = PRESSED;
        ctx->longPressTime = millis();
        if (ctx->pressedCb != NULL) {
            ctx->pressedCb(ctx->arg);
        }
    } else if (pressed && ctx->state == PRESSED && ctx->state != LONG_PRESS) {
        unsigned long currentMillis = millis();
         if(currentMillis - ctx->longPressTime >= ctx->longDelayTime) { 
                ctx->longPressTime = currentMillis;
                ctx->state = LONG_PRESS;
                if (ctx->longPressCb != NULL) {
                    ctx->longPressCb(ctx->arg);
                }
            }
    
    } else if (!pressed && ctx->state != IDLE) {
        ctx->state = IDLE;
        ctx->longPressTime = 0;
    }

    return 0;
}

ButtonState Button_If_Pressed(Button_FSM_t *ctx) {
    if (ctx == NULL) {
        Serial.println("Error: FSM pointer or state pointer is NULL");
        return IDLE;
    }

    if (ctx->state == PRESSED) {
        return PRESSED;
    } else if (ctx->state == LONG_PRESS) {
        return LONG_PRESS;
    } else if (ctx->state == RELEASED) {
        return RELEASED;
    } else {
        return IDLE;
    }

}