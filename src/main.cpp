#include <Arduino.h>

#define LED_RED_PIN 15
#define BUTTON_PIN 16
#define BOOT_BUTTON_PIN 0
#define DEBOUNCE_DELAY 5000

bool isPrintedButton = true;
bool isPrintedBoot = true;
bool isPrintedBoth = true;

unsigned long pressTime = 0;
class LedController
{
private:
    int m_redLed{};
    int m_buttonPin{};

public:
    void init(int redLedPin, int buttonPin)
    {
        m_redLed = redLedPin;
        m_buttonPin = buttonPin;
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
    void printOnes(const char *buttonType, bool &isPrinted)
    {
        if (isPrintedButton)
            Serial.printf("%s pressed. LED is ON.\n", buttonType);
        isPrinted = false;
    }

    void buttonMode(uint32_t delayTime, int ledPin = LED_RED_PIN){
            turnOnLed(ledPin);
            delay(delayTime);
            turnOffLed(ledPin);
    }

    // led toggle implementation
    void toggleLed(uint32_t delayTime)
    {
        const int buttonState = getButtonState(m_buttonPin);
        const int bootButtonState = getButtonState(BOOT_BUTTON_PIN);

        Serial.println("Button state: " + String(buttonState) + ", Boot button state: " + String(bootButtonState)  + "\n");

        if (buttonState == LOW && bootButtonState == HIGH)
        {
            unsigned long duration = millis() - pressTime;
            Serial.println("Button pressed. Duration: " + String(duration) + " ms");
            if(duration > DEBOUNCE_DELAY) {
                printOnes("Button", isPrintedButton);
                buttonMode(100, m_redLed);
            } else {
                printOnes("Button", isPrintedButton);
                buttonMode(delayTime, m_redLed);
            }
            
        }
        else if (buttonState == HIGH)
        {
            Serial.println("Button released.");
            isPrintedButton = true;
            pressTime = millis();
        }
        delay(delayTime);
    }
    // boot button toggle implementation
    void bootLedToggle(uint32_t delayTime)
    {
        const int bootButtonState = getButtonState(BOOT_BUTTON_PIN);
        const int buttonState = getButtonState(m_buttonPin);
        if (bootButtonState == LOW && buttonState == HIGH)
        {
            printOnes("Boot", isPrintedBoot);
            buttonMode(delayTime, m_redLed);
        }
        delay(delayTime);
    }

    void toggleLedByTwoButtons(uint32_t delayTime)
    {
        const int buttonState = getButtonState(m_buttonPin);
        const int bootButtonState = getButtonState(BOOT_BUTTON_PIN);

        if (buttonState == LOW && bootButtonState == LOW)
        {
            printOnes("Both", isPrintedBoth);
            buttonMode(delayTime, m_redLed);
        }
        delay(delayTime);
    }
};

LedController ledController{};

void setup()
{
    Serial.begin(115200);
    delay(1000);

    pinMode(LED_RED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

    Serial.println("Setup complete. Starting loop...");

    ledController.init(LED_RED_PIN, BUTTON_PIN);
}


void loop()
{
    ledController.toggleLed(2000);
    ledController.bootLedToggle(1000);
    ledController.toggleLedByTwoButtons(200);
}
