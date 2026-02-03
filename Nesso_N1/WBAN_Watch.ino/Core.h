#ifndef CORE_H
#define CORE_H

#include "Config.h"
#include <Arduino_Nesso_N1.h> // Handles Power & Backlight

class CoreManager {
  public:
    int hour = 10;
    int minute = 30;
    int batteryLevel = 85; // Placeholder until FW v3.3.5
    bool isScreenOn = true;

    void begin() {
      // Nesso.begin() is automatic, but we init specific power pins here
      pinMode(LCD_BACKLIGHT, OUTPUT);
      wakeScreen(); // Start ON
    }

    void updateClock() {
      static unsigned long lastTick = 0;
      if (millis() - lastTick > 60000) { // Every 1 minute
        minute++;
        if (minute > 59) {
          minute = 0;
          hour++;
          if (hour > 23) hour = 0;
        }
        lastTick = millis();
      }
    }

    void wakeScreen() {
      if (!isScreenOn) {
        isScreenOn = true;
        digitalWrite(LCD_BACKLIGHT, HIGH);
        if(SERIAL_DEBUG) Serial.println("Screen WAKE");
      }
    }

    void sleepScreen() {
      if (isScreenOn) {
        isScreenOn = false;
        digitalWrite(LCD_BACKLIGHT, LOW);
        if(SERIAL_DEBUG) Serial.println("Screen SLEEP");
      }
    }
};

extern CoreManager Core;

#endif