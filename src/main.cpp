#include <Arduino.h>
#include <iostream>

#define LED_RED_PIN 15
#define BUTTON_PIN 16

bool showLogsCount = true;

class LedController
{
private:
    bool m_ledState{false};
    int m_blueLed{};
    int m_redLed{};
    int m_buttonPin{};
    bool m_showState{false};

public:
    LedController(int redLedPin, int buttonPin) : m_redLed(redLedPin), m_buttonPin(buttonPin)
    {
    }

    void turnOnLed(int led)
    {
        digitalWrite(led, HIGH);
    }
    void turnOffLed(int led)
    {
        digitalWrite(led, LOW);
    }

    int getButtonState()
    {
        return digitalRead(m_buttonPin);
    }
    bool setLogState(bool currentState)
    {
        if (currentState != m_showState)
        {
            m_showState = currentState;
            showLogsCount = true;
            return true;
        }
        showLogsCount = false;
        return m_showState;
    }
    void toggleLed(int delayTime)
    {
        const int buttonState = getButtonState();

        if (buttonState == LOW)
        {
            turnOffLed(m_redLed);
            if(showLogsCount)Serial.println("Button pressed. LED is ON.");
        }
        else
        {
            turnOnLed(m_redLed);
            if(showLogsCount)Serial.println("Button released. LED is OFF.");
        }
        setLogState(buttonState);
        delay(delayTime);
    }
};

LedController ledController{LED_RED_PIN, BUTTON_PIN};

void setup()
{
    Serial.begin(115200);
    delay(1000);

    pinMode(LED_RED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLDOWN);

    Serial.println("Setup complete. Starting loop...");
}

void loop()
{
    ledController.toggleLed(500);
}
