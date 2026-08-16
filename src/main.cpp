#include <Arduino.h>
#include <iostream>

#define SWITCHER_PIN 15
#define ADC_IN 4
#define ADC_IN_2 5

#define READ_TIME_DELAY 100
#define ADC_MAX_VALUE 4095 // 12-bit ADC resolution
#define U_MAX_VOLTAGE 3100 // Maximum voltage in millivolts for ESP32 ADC based on 
#define HYSTERESIS 100 // Hysteresis value in millivolts
#define ADC_THRESHOLD 2000 // Threshold voltage in millivolts for LED control
#define ADC2_THRESHOLD 2000

class WaterPumpController {
private:
    int m_switcherPin {};
    int m_adcPin {};
    int m_adcPin2 {};

    uint16_t calcSamplesNumber() {
      return (1000 / READ_TIME_DELAY);
  }

public:
  WaterPumpController(int adcPin = ADC_IN, int adcPin2 = ADC_IN_2, int switcherPin = SWITCHER_PIN) : m_adcPin(adcPin), m_adcPin2(adcPin2), m_switcherPin(switcherPin) {
    std::cout << "WaterPumpController initialized with state: " << m_adcPin << std::endl;
    std::cout << "WaterPumpController initialized with state: " << switcherPin << std::endl;
    std::cout << "WaterPumpController initialized with state: " << switcherPin << std::endl; 

  }


  uint32_t readADC_1() {
        int samplesNumber = calcSamplesNumber();
        uint32_t sum {0};// Convert ADC value to millivolts
         for (int i = 0; i<samplesNumber;++i){
        sum += analogReadMilliVolts(m_adcPin);
    }
        return sum / calcSamplesNumber();
  }
  void turnOffWaterPump( int led)  {
      digitalWrite(led, LOW);
  }
  void turnOnWaterPump(int led) {
      digitalWrite(led, HIGH);
  }
  uint32_t readADC_2() {
        int samplesNumber = calcSamplesNumber();
        uint32_t sum {0};// Convert ADC value to millivolts
         for (int i = 0; i<samplesNumber;++i){
        sum += analogReadMilliVolts(m_adcPin2);
    }
        return sum / calcSamplesNumber();
  }
  void toggleWaterPump(int delayTime) {
        uint32_t adc_1 = readADC_1();
        uint32_t adc_2 = readADC_2();
        bool letState {};

        Serial.printf("Value = %1d\n",adc_2);

        if (adc_2 < ADC2_THRESHOLD ){

        if (adc_1 > ADC_THRESHOLD + HYSTERESIS) {
            turnOnWaterPump(SWITCHER_PIN);
        } else if (adc_1 < ADC_THRESHOLD - HYSTERESIS) {
            turnOffWaterPump(SWITCHER_PIN);
        }
      } else if (adc_1 >= ADC2_THRESHOLD) {
            turnOffWaterPump(SWITCHER_PIN);
        }
  }
};

  WaterPumpController waterPumpController{ADC_IN,ADC_IN_2, SWITCHER_PIN};

void setup() {
    Serial.begin(115200);
    delay(1000);

    analogReadResolution(12); // Set ADC resolution to 12 bits to get values from 0 to 4095 (for ESP32)
    

    pinMode(SWITCHER_PIN, OUTPUT);
    digitalWrite(SWITCHER_PIN, LOW);
    pinMode(ADC_IN, INPUT);
    pinMode(ADC_IN_2, INPUT);

}

void loop() {
  delay(500);
  waterPumpController.toggleWaterPump(1000);
}
