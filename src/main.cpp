#include <Arduino.h>

bool buttonPressed{false};
unsigned long timeButtonPress{0};
unsigned long timeSwitchOn{0};

class SwitchConfig
{
public:
    static constexpr uint8_t SWITCH_PIN{15};
    static constexpr uint8_t SWITCH_PIN_MEASURE_INPUT{17};
};

class ButtonConfig
{
public:
    static constexpr u_int8_t BUTTON_PIN{16};
};

class SwitchController
{
private:
    unsigned long previousTime{0};

public:
    void handleSwitch(uint8_t ledPin)
    {
        if (!buttonPressed)
        {
            digitalWrite(ledPin, LOW);
        };

        if (buttonPressed)
        {
            digitalWrite(ledPin, HIGH);
        }
    }

    void handleSwitchOn(uint8_t switchMeasurePin)
    {
        int switchState = digitalRead(switchMeasurePin);
        // measure switch delay
        if (switchState == HIGH)
        {
            if(timeSwitchOn == 0){
                timeSwitchOn = micros();
            // Serial.println("SWITCH ON");
            unsigned long delayTime = timeSwitchOn -timeButtonPress;
            if(delayTime < 3000) {
                Serial.println(delayTime);
            }
            }
        }
        else
        {
            // Serial.println("SWITCH OFF");
            timeSwitchOn = 0;
        }
    }

    void toggleButton(uint8_t buttonPin)
    {
        int buttonState = digitalRead(buttonPin);

        if (buttonState == LOW)
        {
            buttonPressed = true;
            if(timeButtonPress == 0){
                timeButtonPress = micros();
            }
        }
        else
        {
            buttonPressed = false;
            timeButtonPress = 0;
        }
    }
};

SwitchController switchController;
SwitchConfig switchConfig;
ButtonConfig buttonConfig;

void setup()
{
    Serial.begin(115200);
    delay(1000);

    pinMode(switchConfig.SWITCH_PIN, OUTPUT);
    pinMode(switchConfig.SWITCH_PIN_MEASURE_INPUT, INPUT_PULLUP);
    pinMode(buttonConfig.BUTTON_PIN, INPUT_PULLUP);
    digitalWrite(buttonConfig.BUTTON_PIN, LOW);
    Serial.println("Setup complete. Starting loop...");
}

void loop()
{

    switchController.toggleButton(buttonConfig.BUTTON_PIN);
    switchController.handleSwitch(switchConfig.SWITCH_PIN);
    switchController.handleSwitchOn(switchConfig.SWITCH_PIN_MEASURE_INPUT);
}
