#ifndef CONNECTIVITY_H
#define CONNECTIVITY_H

#include "Config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <HardwareSerial.h>

// --- PINS (MATCHING YOUR WORKING SAFE-SEND CODE) ---
#define GSM_RX_PIN 2   
#define GSM_TX_PIN 7   
#define GPS_RX_PIN 4   
#define GPS_TX_PIN -1
#define PHONE_NUMBER "+916381146811" 

class ConnectivityManager {
  private:
    HardwareSerial gsmSerial;
    HardwareSerial gpsSerial;
    String latestGpsData = "Scanning...";

  public:
    // Constructor
    ConnectivityManager() : gsmSerial(1), gpsSerial(2) {}

    void begin() {
      // 1. Init WiFi
      if(SERIAL_DEBUG) Serial.println("Initializing WiFi...");
      WiFi.mode(WIFI_STA); 
      WiFi.begin(WIFI_SSID, WIFI_PASS);

      // 2. Init GSM & GPS Hardware
      gsmSerial.begin(115200, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
      gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
      
      // 3. Wake up GSM
      delay(3000); // Give it time to boot
      gsmSerial.println("AT+CMGF=1"); // Text Mode
      delay(1000);
    }

    // Call this in loop() to keep GPS fresh
    void update() {
      while (gpsSerial.available()) {
        String line = gpsSerial.readStringUntil('\n');
        if (line.startsWith("$GNRMC")) {
          latestGpsData = line; 
        }
      }
    }

    // --- THE HYBRID ALERT FUNCTION ---
    void sendPanicAlert(int stepCount, int batteryLevel, String source) {
      bool wifiSent = false;

      // -----------------------------------------
      // PRIORITY 1: WIFI (BACKEND API)
      // -----------------------------------------
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("🚀 Sending via WiFi...");
        
        WiFiClient client;
        HTTPClient http;
        // Construct URL from Config.h
        String url = "http://" + String(SERVER_IP) + ":" + String(SERVER_PORT) + String(API_ENDPOINT);

        if (http.begin(client, url)) {
           http.addHeader("Content-Type", "application/json");
           
           String jsonData = "{";
           jsonData += "\"deviceId\": \"Nesso-001\",";
           jsonData += "\"type\": \"PANIC\",";
           jsonData += "\"source\": \"" + source + "\",";
           jsonData += "\"steps\": " + String(stepCount) + ",";
           jsonData += "\"gps\": \"" + latestGpsData + "\""; 
           jsonData += "}";

           int code = http.POST(jsonData);
           if (code > 0) {
             Serial.println("✅ WiFi Alert Sent!");
             wifiSent = true; 
           } else {
             Serial.println("❌ WiFi Failed. Error: " + String(code));
           }
           http.end();
        }
      } else {
        Serial.println("⚠️ WiFi Disconnected. Skipping...");
      }

      // -----------------------------------------
      // PRIORITY 2: GSM SMS (ALWAYS SEND AS BACKUP)
      // -----------------------------------------
      // Even if WiFi works, we send SMS because it's safer.
      Serial.println("📡 Initiating GSM SMS...");
      sendGsmSms(source);
    }

    // --- THE "SAFE SEND" LOGIC (MATCHING YOUR WORKING CODE) ---
    void sendGsmSms(String source) {
      Serial.println(">>> SENDING SHORT SMS <<<");

      // 1. Check Signal (Optional, creates a small delay)
      gsmSerial.println("AT+CSQ");
      delay(500);

      // 2. Start Command
      gsmSerial.print("AT+CMGS=\"");
      gsmSerial.print(PHONE_NUMBER);
      gsmSerial.println("\"");
      
      // CRITICAL DELAY FROM YOUR WORKING CODE
      delay(2000); 

      // 3. Type Message
      gsmSerial.print("SOS ALERT! Source: ");
      gsmSerial.print(source);
      gsmSerial.print("\nGPS Data:\n");
      delay(100);

      // 4. Smart Link Logic
      if (latestGpsData.indexOf(",V,") > 0 || latestGpsData.indexOf("Scanning") >= 0) {
        gsmSerial.print("https://maps.google.com (Indoors/Scanning)");
      } else {
        gsmSerial.print("https://maps.google.com (Active Lock)");
      }
      
      delay(500);

      // 5. Send Ctrl+Z
      gsmSerial.write(26); 
      Serial.println("✅ SMS Command Sent (Waiting for Network...)");
    }
};

extern ConnectivityManager Connectivity;

#endif