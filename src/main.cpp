#include <Arduino.h>
#include "Button.h"
#include "Led.h"

// LED1 блимає кожні 200 мс

#define LED_1_PIN 15
#define BUTTON_PIN 18

volatile boolean eventOccurred = false;
volatile int interruptCount = 0;
volatile unsigned long lastInterruptTimestamp = 0;
volatile unsigned long lastButtonState = 0;

static Button_FSM_t buttonFsm;
static Led_t led_1;

void onButtonPress(void *arg)
{
    Serial.println("Button pressed");
}

void quickIRS()
{
    eventOccurred = true;
}

void setup()
{
    int retVal = 0;

    Serial.begin(115200);

    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), quickIRS, FALLING);

    retVal = Led_Init(&led_1, LED_1_PIN, OFF, 200, NULL);
    if (retVal != 0)
    {
        Serial.println("Failed to initialize LED 1");
    }

    retVal = Button_Init(&buttonFsm, BUTTON_PIN, IDLE, onButtonPress);
    if (retVal != 0)
    {
        Serial.println("Failed to initialize button FSM");
    }
}

void loop()
{
    int retVal = 0;
    bool buttonState = false;

    retVal = Button_FSM_Update(&buttonFsm);
    if (retVal != 0)
    {
        Serial.println("Failed to update button FSM");
    }

    buttonState = Button_If_Pressed(&buttonFsm);

    if (eventOccurred)
    {
        unsigned long currentTimestamp = millis();
        eventOccurred = false;

        Serial.print("Interrupt occurred at: ");

        if (currentTimestamp - lastInterruptTimestamp > 50 && buttonState)
        {
            lastInterruptTimestamp = currentTimestamp;
            Serial.println("Button pressed, updating LED 1");

            retVal = Led_Update(&led_1, buttonState);
            if (retVal != 0)
            {
                Serial.println("Failed to update LED 1");
            }
        }
    }
}
