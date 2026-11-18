#include "Semaphore.h"
#include <Arduino.h>
#define LED_1_PIN 			15
#define LED_2_PIN 			16
#define LED_3_PIN 			17

int Semaphore_Init(Semaphore_t *ctx) {
    if (ctx == NULL) {
        return -1;
    }

    int retVal = 0;
    retVal |= Led_Init(&ctx->led_green, LED_1_PIN, OFF, 200, NULL);
    retVal |= Led_Init(&ctx->led_yellow, LED_2_PIN, OFF, 200, NULL);
    retVal |= Led_Init(&ctx->led_red, LED_3_PIN, OFF, 200, NULL);
    ctx->state = NONE;
    ctx->lastTime = millis();
    ctx->pressedCb = NULL;
    ctx->triggerCb = NULL;
    ctx->arg = NULL;

    return retVal;
}


// 1. Фаза "Рух дозволено" (Зелений):
// Горить лише зелений світлодіод протягом 5 секунд. Програма в цей час не спить, а
// постійно перевіряє таймер millis().
// 2. Фаза "Увага, скоро стоп" (Зелений миготливий):
// Зелений починає ритмічно вмикатися та вимикатися (кожні 500 мс). Це триває 3 секунди. Це
// найскладніша частина для коду без delay, бо ми рахуємо і загальний час миготіння, і
// інтервал кожного спалаху одночасно.
// 3. Фаза "Заборона руху" (Жовтий):
// Зелений гасне, на 2 секунди вмикається сталий жовтий. Це сигнал, що перехрестя треба
// звільнити.
// 4. Фаза "Стоп" (Червоний):
// Горить лише червоний світлодіод протягом 5 секунд. Повний спокій для водіїв.
// 5. Фаза "Приготуватися" (Червоний + Жовтий):
// Не вимикаючи червоний, додається жовтий. Вони горять разом 2 секунди. Це специфіка
// ПДР України - підготовка до старту.

int Semaphore_Update(Semaphore_t *ctx, ButtonState buttonState) {
    if (ctx == NULL) {
        return -1;
    }
    int retVal = 0;

    if (buttonState == PRESSED) {
        ctx->state = GREEN;
        ctx->lastTime = millis();
    } else if (buttonState == LONG_PRESS) {
        ctx->lastTime = 0;
        ctx->state = NONE;
        retVal |= Led_Update(&ctx->led_green, OFF);
        retVal |= Led_Update(&ctx->led_yellow, OFF);
        retVal |= Led_Update(&ctx->led_red, OFF);
    }

    unsigned long now = millis();

    
    switch (ctx->state) {
        case GREEN:
            retVal |= Led_Update(&ctx->led_green, ON);
            if (now - ctx->lastTime >= 5000) {
                ctx->state = GREEN_BLINK;
                ctx->lastTime = now;
            }
            break;
        case GREEN_BLINK:
            retVal |= Led_Update(&ctx->led_green, BLINK);
            if (now - ctx->lastTime >= 3000) {
                ctx->state = YELLOW;
                ctx->lastTime = now;
                retVal |= Led_Update(&ctx->led_green, OFF);
                retVal |= Led_Update(&ctx->led_yellow, ON);
                retVal |= Led_Update(&ctx->led_red, OFF);
            }
            break;
        case YELLOW:
            if (now - ctx->lastTime >= 2000) {
                ctx->state = RED;
                ctx->lastTime = now;
                retVal |= Led_Update(&ctx->led_green, OFF);
                retVal |= Led_Update(&ctx->led_yellow, OFF);
                retVal |= Led_Update(&ctx->led_red, ON);
            }
            break;
        case RED:
            if (now - ctx->lastTime >= 5000) {
                ctx->state = RED_YELLOW;
                ctx->lastTime = now;
                retVal |= Led_Update(&ctx->led_green, OFF);
                retVal |= Led_Update(&ctx->led_yellow, ON);
                retVal |= Led_Update(&ctx->led_red, ON);
            }
            break;
        case RED_YELLOW:
            if (now - ctx->lastTime >= 2000) {
                ctx->state = GREEN;
                ctx->lastTime = now;
                retVal |= Led_Update(&ctx->led_green, ON);
                retVal |= Led_Update(&ctx->led_yellow, OFF);
                retVal |= Led_Update(&ctx->led_red, OFF);
            }
            break;
        default:
            return -1; // Invalid state
    }
    return retVal;
}