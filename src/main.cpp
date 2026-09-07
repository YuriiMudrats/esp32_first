#include <Arduino.h>
#include "Button.h"
#include "Led.h"

// LED1 блимає кожні 200 мс
// LED2 блимає кожні 500 мс
// LED3 блимає кожні 1000 мс

#define LED_1_PIN 			15
#define LED_2_PIN 			16
#define LED_3_PIN 			17
#define BUTTON_PIN          18


static Button_FSM_t buttonFsm;
static Led_t led_1;
static Led_t led_2;
static Led_t led_3;

void onButtonPress(void* arg) {
    Serial.println("Button pressed");
}

void onButtonLongPress(void* arg) {
    Serial.println("Button pressed for a long time");
}

void setup() {
    int retVal = 0;

    Serial.begin(115200);



    retVal = Led_Init(&led_1, LED_1_PIN, OFF, 200, NULL);
    if (retVal != 0) {
        Serial.println("Failed to initialize LED 1");
    }

    retVal = Led_Init(&led_2, LED_2_PIN, OFF, 500, NULL);
    if (retVal != 0) {
        Serial.println("Failed to initialize LED 2");
    }

    retVal = Led_Init(&led_3, LED_3_PIN, OFF, 1000, NULL);
    if (retVal != 0) {
        Serial.println("Failed to initialize LED 3");
    }

    retVal = Button_Init(&buttonFsm, BUTTON_PIN, IDLE, onButtonPress, onButtonLongPress);
    if (retVal != 0) {
        Serial.println("Failed to initialize button FSM");
    }
}

void loop() {
    int retVal = 0;
    LedState mode = OFF;
    ButtonState buttonState = IDLE;

    retVal = Button_Update(&buttonFsm);
    if (retVal != 0) {
        Serial.println("Failed to update button FSM");
    }

    buttonState = Button_If_Pressed(&buttonFsm);

    if (buttonState == PRESSED) {
        mode = ON;
    } else if (buttonState == LONG_PRESS) {
        mode = BLINK;
    } else {
         mode = OFF;
    }

    retVal = Led_Update(&led_1, mode);
    if (retVal != 0) {
        Serial.println("Failed to update LED 1");
    }

    retVal = Led_Update(&led_2, mode);
    if (retVal != 0) {
        Serial.println("Failed to update LED 2");
    }

    retVal = Led_Update(&led_3, mode);
    if (retVal != 0) {
        Serial.println("Failed to update LED 3");
    }
}
