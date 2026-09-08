#include <Arduino.h>
#include "Button.h"
#include "Semaphore.h"

#define BUTTON_PIN          18

static Semaphore_t semaphore;
static Button_FSM_t buttonFsm;

void onButtonPress(void* arg) {
    Serial.println("Button pressed");
}

void onButtonLongPress(void* arg) {
    Serial.println("Button pressed for a long time");
}

void setup() {
    int retVal = 0;
    int semVal = 0;

    Serial.begin(115200);



   semVal = Semaphore_Init(&semaphore);
    if (semVal != 0) {
        Serial.println("Failed to initialize semaphore");
    }

    retVal = Button_Init(&buttonFsm, BUTTON_PIN, IDLE, onButtonPress, onButtonLongPress);
    if (retVal != 0) {
        Serial.println("Failed to initialize button FSM");
    }
}

void loop() {
    int retVal = 0;
    ButtonState buttonState = IDLE;

    retVal = Button_Update(&buttonFsm);
    if (retVal != 0) {
        Serial.println("Failed to update button FSM");
    }

    buttonState = Button_If_Pressed(&buttonFsm);

    retVal = Semaphore_Update(&semaphore, buttonState);
    if (retVal != 0) {
        Serial.println("Failed to update semaphore");
    }
}
