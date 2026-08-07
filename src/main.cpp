#include <Arduino.h>

#define LED_RED 15
#define LED_BLUE 16

bool ledState = false;

class LedController {
  static bool ledState;
public:
  void turnOnLed(int led) {
      digitalWrite(led, HIGH);
  }
  void turnOffLed( int led)  {
      digitalWrite(led, LOW);
  }
  void toggleLed(int led, int delayTime) {
      if (ledState) {
          turnOffLed(led);
          ledState = false;
      } else {
          turnOnLed(led);
          ledState = true;
      }
       delay(delayTime);
  }
};

void setup() {
    Serial.begin(115200);
    delay(1000);
    

    pinMode(LED_RED, OUTPUT);
    digitalWrite(LED_RED, LOW);
    pinMode(LED_BLUE, OUTPUT);
    digitalWrite(LED_BLUE, HIGH);
}

void loop() {
  LedController ledControllerRed{};
  LedController ledControllerBlue{};
  ledControllerRed.toggleLed(LED_RED, 1000);
  ledControllerBlue.toggleLed(LED_BLUE, 1000);
}
