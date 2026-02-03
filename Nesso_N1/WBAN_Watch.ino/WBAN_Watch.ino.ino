/*
 * WBAN Smartwatch - VERBOSE DEBUG EDITION
 * ---------------------------------------
 * PRINTS EVERYTHING:
 * 1. WiFi Status
 * 2. ESP-NOW Data (from Dress)
 * 3. IMU Readings (X, Y, Z)
 * 4. AI Model Confidence Scores
 */

#include "Config.h"
#include "Core.h"
#include "Sensors.h"
#include "UI.h"
#include "Connectivity.h"
#include <Arduino_BMI270_BMM150.h>
#include <Women_Safety_Core_inferencing.h> 
#include <esp_now.h>
#include <WiFi.h>
#include <Arduino_Nesso_N1.h> 

// --- GLOBAL OBJECTS ---
CoreManager Core;
SensorManager Sensors;
UIManager UI;
ConnectivityManager Connectivity;

// --- AI SETTINGS ---
#define EI_CLASSIFIER_SENSOR_AXES_COUNT 4 
static float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];

// --- DATA VARIABLES ---
typedef struct struct_message {
  int pressureValue;
} struct_message;

struct_message incomingData;
float currentPressure = 0.0; 

// --- FUNCTION PROTOTYPES ---
void handlePanic(String source);
int raw_feature_get_data(size_t offset, size_t length, float *out_ptr);
void runAILogic();

// --- ESP-NOW CALLBACK ---
void OnDataRecv(const esp_now_recv_info_t * info, const uint8_t *incomingDataPtr, int len) {
  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));
  currentPressure = (float)incomingData.pressureValue;
  
  // DEBUG: Show incoming data instantly
  Serial.print(">>> [ESP-NOW] From Dress | Pressure: "); 
  Serial.println(currentPressure);
}

// -------------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  
  // DELAY TO CATCH SERIAL MONITOR
  delay(3000); 
  Serial.println("\n\n========================================");
  Serial.println("   NESSO N1 - DIAGNOSTIC MODE STARTING   ");
  Serial.println("========================================");

  // 1. UI
  Serial.print("[BOOT] 1. Initializing UI... ");
  UI.begin();
  Serial.println("OK");
  
  // 2. Connectivity
  Serial.print("[BOOT] 2. Initializing WiFi/GSM... ");
  Core.begin();
  Connectivity.begin(); 
  Serial.println("OK");

  // 3. Sensors
  Serial.print("[BOOT] 3. Initializing IMU... ");
  Sensors.begin();
  if (!IMU.begin()) {
    Serial.println("FAIL (Check Hardware!)");
  } else {
    Serial.println("OK");
  }

  // 4. Button
  Serial.print("[BOOT] 4. Initializing Button (KEY1)... ");
  pinMode(KEY1, INPUT_PULLUP);
  Serial.println("OK");

  // 5. ESP-NOW
  Serial.print("[BOOT] 5. Initializing ESP-NOW... ");
  if (esp_now_init() != ESP_OK) {
    Serial.println("FAIL!");
  } else {
    esp_now_register_recv_cb(OnDataRecv);
    Serial.println("OK (Listening for Dress)");
  }

// ... (Keep the rest of setup above this) ...

  Serial.println("----------------------------------------");
  Serial.print("[INFO] WiFi MAC Address: "); Serial.println(WiFi.macAddress());
  Serial.print("[INFO] WiFi Channel: "); Serial.println(WiFi.channel()); // <--- NEW LINE
  Serial.print("[INFO] WiFi Status: "); Serial.println(WiFi.status() == WL_CONNECTED ? "CONNECTED" : "DISCONNECTED");
  Serial.println("----------------------------------------");
  Serial.println("Waiting for Data Stream...");
}


// -------------------------------------------------------------------------
// MAIN LOOP
// -------------------------------------------------------------------------
void loop() {
  // 1. Update Basics
  Core.updateClock();
  UI.update();
  Connectivity.update();

  // 2. Check SOS Button
  if (digitalRead(KEY1) == LOW) { 
    handlePanic("BUTTON");
    delay(500); 
    return; 
  }

  // 3. Run AI (This includes the Sensor Read & Debug Print)
  runAILogic();
}

// -------------------------------------------------------------------------
// AI LOGIC & DEBUG PRINTING
// -------------------------------------------------------------------------
void runAILogic() {
  // Collection Loop: Fills buffer with 1 second of data
  // We print the FIRST reading of every batch so you can see the sensor values
  bool debugPrinted = false;

  for (int i = 0; i < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; i += EI_CLASSIFIER_SENSOR_AXES_COUNT) {
      float x, y, z;
      unsigned long startMicros = micros(); 

      // Read Sensor
      if (IMU.accelerationAvailable()) {
          IMU.readAcceleration(x, y, z);
          
          // DEBUG: Print Raw Sensors once per cycle
          if (!debugPrinted) {
            Serial.print("[SENSORS] X:"); Serial.print(x);
            Serial.print(" Y:"); Serial.print(y);
            Serial.print(" Z:"); Serial.print(z);
            Serial.print(" | Dress Press:"); Serial.println(currentPressure);
            debugPrinted = true;
          }

          // Fill Features
          features[i + 0] = x;
          features[i + 1] = y;
          features[i + 2] = z;
          features[i + 3] = currentPressure; 
      }

      // Exact 50Hz Timing
      while (micros() - startMicros < 20000); 
  }

  // Run Inference
  ei_impulse_result_t result = { 0 };
  signal_t features_signal;
  features_signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
  features_signal.get_data = &raw_feature_get_data;

  EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false);

  if (res == EI_IMPULSE_OK) {
      // PRINT AI RESULTS
      Serial.print("[AI RESULT] ");
      float panic_score = 0.0;
      
      for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
          Serial.print(result.classification[ix].label);
          Serial.print(": ");
          Serial.print(result.classification[ix].value);
          Serial.print(" | ");
          
          if (String(result.classification[ix].label) == "panic") {
            panic_score = result.classification[ix].value;
          }
      }
      Serial.println(); // Newline

      // TRIGGER ALERT
      if (panic_score > 0.85) {
          handlePanic("AI_FALL");
      }
  }
}

// -------------------------------------------------------------------------
// ALERT HANDLER
// -------------------------------------------------------------------------
void handlePanic(String source) {
  Serial.print("🚨 PANIC TRIGGERED BY: "); Serial.println(source);
  UI.currentPage = PAGE_PANIC;
  UI.update(); 
  Connectivity.sendPanicAlert(Sensors.stepCount, Core.batteryLevel, source);
  
  unsigned long startPanic = millis();
  while (millis() - startPanic < 5000) {
    UI.update(); 
    delay(100);
  }
  UI.currentPage = PAGE_CLOCK;
  UI.update();
}

int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}