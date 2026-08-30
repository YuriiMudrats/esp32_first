#include <Arduino.h>

enum LightState
{
    BLINK,
    ON,
    OFF
};
volatile bool buttonPressed{false};
volatile int printCount{0};
volatile bool ledState{false};

LightState mode = BLINK;

class LedConfig
{
public:
    static constexpr u_int8_t LED_PIN{15};
    static const int LED_INTERVAL{300};
    static const int LED_MODE_CHANGE_INTERVAL{3000};
};

class ButtonConfig
{
public:
    static constexpr u_int8_t BUTTON_PIN{16};
};

void printLog(char *text)
{
    printCount++;
    if (printCount % 100000 == 0)
    {
        Serial.println(text);
    }
}

class LedController
{
private:
    static const int redLed{};
    static const int m_buttonPin{};
    static const int m_delay{};

    unsigned long previousTime{0};
    unsigned long previousButtonTime{0};

public:
    void blinkMode(uint8_t ledPin, int interval)
    {
        if (mode != BLINK)
        {
            return;
        }

        unsigned long now = millis();
        if (now - previousTime >= interval)
        {
            previousTime = now;
            ledState = !ledState;
            digitalWrite(ledPin, ledState ? HIGH : LOW);
        }
    }

    void handleLed(uint8_t ledPin, const int interval, const int modeInterval)
    {
        if (!buttonPressed)
        {
            digitalWrite(ledPin, LOW);
        };

        if (buttonPressed)
        {
            unsigned long now = millis();
            if (now - previousButtonTime >= modeInterval)
            {
                previousButtonTime = now;
                mode = static_cast<LightState>((mode + 1)%3);
            }
            if (mode == ON)
            {
                digitalWrite(ledPin, HIGH);
            }
            else if (mode == BLINK)
            {
                blinkMode(ledPin,interval);
            } else if (mode == OFF) {
                digitalWrite(ledPin, LOW);
            }
        }
    }

    void toggleLed(uint8_t buttonPin)
    {
        int buttonState = digitalRead(buttonPin);

        if (buttonState == LOW)
        {
            buttonPressed = true;
            printLog("LED ON");
        }
        else
        {
            buttonPressed = false;
            printLog("LED OFF");
        }
    }
};

LedController ledController{};
LedConfig ledConfig{};
ButtonConfig buttonConfig{};

void setup()
{
    Serial.begin(115200);
    delay(1000);

    pinMode(ledConfig.LED_PIN, OUTPUT);
    pinMode(buttonConfig.BUTTON_PIN, INPUT_PULLUP);
    digitalWrite(buttonConfig.BUTTON_PIN, LOW);
    Serial.println("Setup complete. Starting loop...");
}

void loop()
{
    ledController.toggleLed(buttonConfig.BUTTON_PIN);
    ledController.handleLed(ledConfig.LED_PIN, ledConfig.LED_INTERVAL, ledConfig.LED_MODE_CHANGE_INTERVAL);
}
