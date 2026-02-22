#ifndef CORE_H
#define CORE_H

#include "Config.h"
#include <Arduino_Nesso_N1.h>

class CoreManager {
  public:
    NessoBattery battery;

    int hour = 0;
    int minute = 0;
    int second = 0;
    bool timeSynced = false;
    int batteryLevel = 0; 
    float batteryVoltage = 0.0; // NEW: Track raw voltage
    bool isCharging = false;
    bool isScreenOn = true;
    bool isLowBattery = false;

    // --- SCHEDULER TIMERS ---
    unsigned long lastScreenInteraction = 0;
    unsigned long lastBatteryCheck = 0;
    unsigned long lastClockUpdate = 0;

    void begin() {
      battery.begin();
      battery.enableCharge();

      pinMode(LCD_BACKLIGHT, OUTPUT);
      wakeScreen(); 

      updateBattery();
      lastClockUpdate = millis(); // Start the stopwatch
    }

    void setTime(int h, int m, int s) {
      hour = h;
      minute = m;
      second = s;
      timeSynced = true;
      lastClockUpdate = millis(); // perfectly sync the stopwatch with the real world
    }

    void updateTasks() {
      unsigned long now = millis();

      // 1. Clock Scheduler (NEW: CATCH-UP LOGIC)
      // If the AI took 3 seconds, this while loop runs 3 times instantly!
      while (now - lastClockUpdate >= 1000) { 
        advanceClock();
        lastClockUpdate += 1000; // Add exactly 1000ms, do NOT reset to 'now'
      }

      // 2. Battery Scheduler
      if (now - lastBatteryCheck > BATTERY_CHECK_RATE) {
        updateBattery();
        lastBatteryCheck = now;
      }

      // 3. Screen Timeout Scheduler
      if (isScreenOn && (now - lastScreenInteraction > SCREEN_TIMEOUT)) {
        sleepScreen();
      }
    }

    void advanceClock() {
      second++;
      if (second > 59) {
        second = 0;
        minute++;
        if (minute > 59) {
          minute = 0;
          hour++;
          if (hour > 23) hour = 0;
        }
      }
    }

    void updateBattery() {
      batteryLevel = battery.getChargeLevel();
      // NEW: Read the actual voltage from the Nesso battery controller
      batteryVoltage = battery.getVoltage(); 
      
      isCharging = (battery.getChargeStatus() == NessoBattery::CHARGING);

      if (batteryLevel > 100) batteryLevel = 100;
      if (batteryLevel < 0) batteryLevel = 0;

      // Low Battery Detection Logic (Triggers at 15%)
      if (batteryLevel <= BATTERY_LOW_ALARM && !isCharging) {
        isLowBattery = true;
      } else {
        isLowBattery = false;
      }
    }

    void wakeScreen() {
      isScreenOn = true;
      lastScreenInteraction = millis(); // Reset timeout timer
      digitalWrite(LCD_BACKLIGHT, HIGH); 
      if(SERIAL_DEBUG) Serial.println("💡 Screen WAKE");
    }

    void sleepScreen() {
      isScreenOn = false;
      digitalWrite(LCD_BACKLIGHT, LOW);
      if(SERIAL_DEBUG) Serial.println("💤 Screen SLEEP");
    }
    
    void resetScreenTimeout() {
      if (!isScreenOn) wakeScreen();
      lastScreenInteraction = millis();
    }
};

extern CoreManager Core;

#endif