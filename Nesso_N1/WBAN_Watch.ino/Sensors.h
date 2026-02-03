#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino_BMI270_BMM150.h>
#include "Config.h"

class SensorManager {
  public:
    int stepCount = 0;
    
    void begin() {
      if (!IMU.begin()) {
        if(SERIAL_DEBUG) Serial.println("❌ IMU Failed!");
      } else {
        if(SERIAL_DEBUG) Serial.println("✅ IMU Initialized");
      }
    }

    void update() {
      // Background Step Detection
      if (IMU.accelerationAvailable()) {
        float x, y, z;
        IMU.readAcceleration(x, y, z);
        
        // Calculate Magnitude
        float mag = sqrt(x*x + y*y + z*z);
        
        // Simple Pedometer Logic
        // Uses a non-blocking timer to prevent double-counting steps
        static unsigned long lastStepTime = 0;
        if (mag > STEP_THRESHOLD && (millis() - lastStepTime > 300)) {
          stepCount++;
          lastStepTime = millis();
          if(SERIAL_DEBUG) Serial.println("Step Detected!");
        }
      }
    }
};

extern SensorManager Sensors; // Make available globally

#endif