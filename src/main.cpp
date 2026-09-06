#include <Arduino.h>
#include "Switch.h"

#define SWITCH_1_PIN 15

hw_timer_t *timer_1 = NULL;

volatile boolean eventOccurred = false;
volatile bool switchActive = false;
unsigned long lastUpdateTime = 0;
int debounceDelay = 5000; // milliseconds


static Switch_t switch_1;

void IRAM_ATTR onTimer()
{
    Serial.println("Switch state changed");
    switchActive = true; // Reset the switch state after timer interrupt
    lastUpdateTime = millis(); // Update the last update time to current time
}

void quickIRS(void *arg)
{
    timerAlarmEnable(timer_1);
}

void setup()
{
    int retVal = 0;

    Serial.begin(115200);

    retVal = Switch_Init(&switch_1, SWITCH_1_PIN, OFF, 100, NULL);
    if (retVal != 0)
    {
        Serial.println("Failed to initialize switch 1");
    }

    timer_1 = timerBegin(0, 80, true);
    timerAttachInterrupt(timer_1, &onTimer, true);// 5 seconds
    timerAlarmWrite(timer_1, 15000000, true);
    timerAlarmEnable(timer_1);
}

void loop()
{
    int retVal = 0;

    if (millis() - lastUpdateTime >= debounceDelay) {
        lastUpdateTime = millis();
        switchActive = false; // Reset the switch state after debounce delay
        Serial.print("Switch state toggled "); // Enable the timer alarm to trigger the interrupt
    }
    retVal = Switch_Update(&switch_1, switchActive);
    if (retVal != 0)   
    {
        Serial.println("Failed to update switch 1");
    }
}
