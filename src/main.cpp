#include <Arduino.h>
#include <iostream>

#define LED_RED 15
#define LED_BLUE 16


class LedController {
private:
    bool m_ledState {};
    int m_blueLed {};
    int m_redLed {};
public:
  LedController(bool ledState = false, int blueLedPin = LED_BLUE, int redLedPin = LED_RED) : m_ledState(ledState), m_blueLed(blueLedPin), m_redLed(redLedPin) {
    std::cout << "LedController initialized with state: " << m_ledState << std::endl; 
  }
  

  void turnOnLed(int led) {
      digitalWrite(led, HIGH);
  }
  void turnOffLed( int led)  {
      digitalWrite(led, LOW);
  }
  void toggleLed(int delayTime) {
    
      if (m_ledState) {
          turnOffLed(m_blueLed);
          turnOnLed(m_redLed);
          m_ledState = false;
      } else {
          turnOffLed(m_redLed);
          turnOnLed(m_blueLed);
          m_ledState = true;
      }
       delay(delayTime);
  }
};

  LedController ledController{true , LED_BLUE, LED_RED};

void setup() {
    Serial.begin(115200);
    delay(1000);
    

    pinMode(LED_RED, OUTPUT);
    digitalWrite(LED_RED, LOW);
    pinMode(LED_BLUE, OUTPUT);
    digitalWrite(LED_BLUE, HIGH);

}

void loop() {
  ledController.toggleLed(1000);
}
