/*
 * WBAN Smartwatch - DEBUG MODE (ESP-NOW VISIBLE)
 * ----------------------------------------------
 * 1. AI Fall Detection
 * 2. GUI & Connectivity
 * 3. GPS Tracking
 * 4. MANUAL TRIGGER: Double-Press Green Button (KEY1)
 * 5. DEBUG: Prints Dress Pressure to Serial Monitor
 */

#include "Config.h"
#include "Core.h"
#include "Sensors.h"
#include "UI.h"
#include "Connectivity.h"
#include <Arduino_BMI270_BMM150.h>
#include <DNN_1_Dataset_inferencing.h>

//#include <CNN_Dataset1_inferencing.h>
//#include <CNN_LSTM_Dataset1_inferencing.h>
//#include <FT_MLP_Dataset1_inferencing.h>
//#include <TCN_Dataset1_inferencing.h>
//#include <ResNet_BiLSTM_Dataset1_inferencing.h>

//#include <DNN_2_Dataset_inferencing.h>
//#include <CNN_Dataset2_inferencing.h>
//#include <CNN_LSTM_Dataset2_inferencing.h>
//#include <FT_MLP_Dataset2_inferencing.h>
//#include <TCN_Dataset2_inferencing.h>
//#include <ResNet_BiLSTM_Dataset2_inferencing.h>

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
void checkManualSOS(); 
int raw_feature_get_data(size_t offset, size_t length, float *out_ptr);
void runAILogic();

// --- ESP-NOW CALLBACK (DEBUG ENABLED) ---
void OnDataRecv(const esp_now_recv_info_t * info, const uint8_t *incomingDataPtr, int len) {
  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));
  currentPressure = (float)incomingData.pressureValue;
  
  // *** DEBUG LINES UNCOMMENTED ***
  Serial.print(">>> [ESP-NOW] From Dress | Pressure: "); 
  Serial.println(currentPressure);
}

// -------------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(3000); 
  Serial.println("\n\n========================================");
  Serial.println("   NESSO N1 - DEBUG FIRMWARE STARTING    ");
  Serial.println("========================================");

  // 1. UI
  Serial.print("[BOOT] 1. Initializing UI... ");
  UI.begin();
  Serial.println("OK");
  
  // 2. Connectivity & Core
  Serial.print("[BOOT] 2. Initializing WiFi/GSM & Core... ");
  Core.begin();
  Connectivity.begin(); 
  
  // ---> SYNC TIME USING WIFI <---
  Connectivity.syncTimeWithWiFi(); 
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

  Serial.println("----------------------------------------");
  Serial.println("✅ SYSTEM READY - Waiting for Data...");
}

// -------------------------------------------------------------------------
// MAIN LOOP
// -------------------------------------------------------------------------
void loop() {
  Core.updateTasks();
  UI.update();
  Connectivity.update();

  checkManualSOS();
  
  // ---> LOW BATTERY OVERRIDE <---
  if (Core.isLowBattery && UI.currentPage != PAGE_PANIC) {
    UI.currentPage = PAGE_LOW_BATT;
  } else if (!Core.isLowBattery && UI.currentPage == PAGE_LOW_BATT) {
    UI.currentPage = PAGE_CLOCK; // Recover when plugged in
  }

  runAILogic();
}

// -------------------------------------------------------------------------
// MANUAL BUTTON LOGIC (DOUBLE TAP TO SOS, SINGLE TAP TO WAKE)
// -------------------------------------------------------------------------
bool lastBtnState = HIGH; 
unsigned long lastPressTime = 0;

void checkManualSOS() {
  int reading = digitalRead(KEY1);

  if (reading == LOW && lastBtnState == HIGH) {
      unsigned long now = millis();

      // ---> WAKE UP THE SCREEN ON ANY PRESS <---
      Core.resetScreenTimeout(); 

      // Check for Double-Tap SOS
      if (now - lastPressTime > 100 && now - lastPressTime < 800) {
          handlePanic("MANUAL_SOS");
          lastPressTime = 0; 
      } else {
          lastPressTime = now;
      }
  }
  lastBtnState = reading;
}

// -------------------------------------------------------------------------
// AI LOGIC (WITH HARDWARE BENCHMARKING & TOUCH POLLING)
// -------------------------------------------------------------------------
void runAILogic() {
  for (int i = 0; i < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; i += EI_CLASSIFIER_SENSOR_AXES_COUNT) {
      float x, y, z;
      unsigned long startMicros = micros(); 

      // --- CHECK TASKS WHILE GATHERING DATA ---
      checkManualSOS();
      UI.handleTouch(); // <--- INSTANT TOUCH RESPONSE FIX 

      if (IMU.accelerationAvailable()) {
          IMU.readAcceleration(x, y, z);
          features[i + 0] = x;
          features[i + 1] = y;
          features[i + 2] = z;
          features[i + 3] = currentPressure; 
      }

      while (micros() - startMicros < 20000); 
  }

  ei_impulse_result_t result = { 0 };
  signal_t features_signal;
  features_signal.total_length = EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
  features_signal.get_data = &raw_feature_get_data;

  // --- START BENCHMARK TIMER ---
  unsigned long startInference = millis();

  EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false);

  // --- STOP BENCHMARK TIMER ---
  unsigned long endInference = millis();

  if (res == EI_IMPULSE_OK) {
      // PRINT CALIBRATION DATA TO SERIAL MONITOR
      Serial.println("\n--- 📊 HARDWARE CALIBRATION REPORT ---");
      Serial.print("⏱️ Total Inference Time: "); Serial.print(endInference - startInference); Serial.println(" ms");
      Serial.print("⚙️  DSP Processing Time:  "); Serial.print(result.timing.dsp); Serial.println(" ms");
      Serial.print("🧠 Neural Net Time:      "); Serial.print(result.timing.classification); Serial.println(" ms");
      Serial.print("💾 Free RAM (Heap):      "); Serial.print(ESP.getFreeHeap() / 1024); Serial.println(" KB");
      Serial.println("----------------------------------------");

      float panic_score = 0.0;
      for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
          if (String(result.classification[ix].label) == "panic") {
            panic_score = result.classification[ix].value;
          }
      }

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