#include <Arduino.h>
#include <iostream>
#include <chrono>
#include <thread>

#define LED_RED_PIN 15
#define BUTTON_PIN 16
#define BOOT_BUTTON_PIN 0

bool showLogs = true;

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

    int getButtonState(int pin)
    {
        return digitalRead(pin);
    }
    bool setLogState(bool currentState)
    {
        if (currentState != m_showState)
        {
            m_showState = currentState;
            showLogs = true;
            return true;
        }
        showLogs = false;
        return m_showState;
    }
    // led toggle implementation
    void toggleLed(int delayTime)
    {
        const int buttonState = getButtonState(m_buttonPin);
         const int bootButtonState = getButtonState(BOOT_BUTTON_PIN);

        if (buttonState == LOW && bootButtonState == HIGH)
        {
            turnOnLed(m_redLed);
            delay(delayTime);
            turnOffLed(m_redLed);
            if(showLogs)Serial.println("Button pressed. LED is ON.");
        } 
        setLogState(buttonState);
        delay(delayTime);
    }
    // boot button toggle implementation 
    void bootLedToggle(int delayTime)
    {
        const int bootButtonState = getButtonState(BOOT_BUTTON_PIN);
        const int buttonState = getButtonState(m_buttonPin);
        if (bootButtonState == LOW && buttonState == HIGH)
        {
            turnOnLed(m_redLed);
            delay(delayTime);
            turnOffLed(m_redLed);
            if(showLogs)Serial.println("Boot button pressed. LED is ON.");
        }   
        setLogState(bootButtonState);
        delay(delayTime);
    }

    void toggleLedByTwoButtons(int delayTime)
    {
        const int buttonState = getButtonState(m_buttonPin);
        const int bootButtonState = getButtonState(BOOT_BUTTON_PIN);

        if (buttonState == LOW && bootButtonState == LOW)
        {
            turnOnLed(m_redLed);
            delay(delayTime);
            turnOffLed(m_redLed);
            if(showLogs)Serial.println("Both buttons pressed. LED is ON.");
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
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

    Serial.println("Setup complete. Starting loop...");
}

void loop()
{

    ledController.toggleLed(2000);
    ledController.bootLedToggle(1000);
    ledController.toggleLedByTwoButtons(200);
}
