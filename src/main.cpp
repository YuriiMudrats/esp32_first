#include <Arduino.h>
#include <iostream>

#define LED_RED 15
#define ADC_IN 4

#define READ_TIME_DELAY 100
#define ADC_MAX_VALUE 4095 // 12-bit ADC resolution
#define U_MAX_VOLTAGE 3100 // Maximum voltage in millivolts for ESP32 ADC based on 
#define HYSTERESIS 100 // Hysteresis value in millivolts
#define ADC_THRESHOLD 2000 // Threshold voltage in millivolts for LED control

class LedController {
private:
    int m_redLedPin {};
    int m_adcPin {};

    uint16_t calcSamplesNumber() {
      return (1000 / READ_TIME_DELAY);
  }

public:
  LedController(int adcPin = ADC_IN, int redLedPin = LED_RED) : m_adcPin(adcPin), m_redLedPin(redLedPin) {
    std::cout << "LedController initialized with state: " << m_adcPin << std::endl;
    std::cout << "LedController initialized with state: " << m_redLedPin << std::endl; 
  }


  uint32_t readMilliVoltsViaAnalogRead () {
    uint32_t sum {0};
    for (int i = 0; i<calcSamplesNumber();++i){
        uint32_t adcValue = (analogRead(m_adcPin) * U_MAX_VOLTAGE) /  ADC_MAX_VALUE; // Convert ADC value to millivolts
        sum += adcValue;
    }
    return sum / calcSamplesNumber();
  }
  void turnOffLed( int led)  {
      digitalWrite(led, LOW);
  }
  void turnOnLed(int led) {
      digitalWrite(led, HIGH);
  }
  uint32_t readMilliVoltsViaAnalogReadMilliVolts() {
        int samplesNumber = calcSamplesNumber();
        uint32_t sum {0};// Convert ADC value to millivolts
         for (int i = 0; i<samplesNumber;++i){
        sum += analogReadMilliVolts(ADC_IN);
    }
        return sum / calcSamplesNumber();
  }
  void toggleLed(int delayTime) {
        uint32_t calcValue = readMilliVoltsViaAnalogRead();
        uint32_t etalonValue = readMilliVoltsViaAnalogReadMilliVolts();
        bool letState {};
       
        float accuracyDeviation = (100 *  (etalonValue - calcValue)) / etalonValue;

        if (etalonValue > ADC_THRESHOLD + HYSTERESIS) {
            turnOnLed(m_redLedPin);
            letState = true;
        } else if (etalonValue < ADC_THRESHOLD - HYSTERESIS) {
            turnOffLed(m_redLedPin);
            letState = false;
        }
        Serial.printf("calc: %lu mV | analog: %lu mV | error: %.3f%%\n",
              (unsigned long)calcValue,
              (unsigned long)etalonValue,
              accuracyDeviation);
  }
};

  LedController ledController{ADC_IN, LED_RED};

void setup() {
    Serial.begin(115200);
    delay(1000);

    analogReadResolution(12); // Set ADC resolution to 12 bits to get values from 0 to 4095 (for ESP32)
    

    pinMode(LED_RED, OUTPUT);
    digitalWrite(LED_RED, LOW);
    pinMode(ADC_IN, INPUT);

}

void loop() {
  delay(500);
  ledController.toggleLed(1000);
}
