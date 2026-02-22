#ifndef CONNECTIVITY_H
#define CONNECTIVITY_H

#include "Config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h> 
#include <HardwareSerial.h>
#include <SoftwareSerial.h> 
#include <time.h> 
#include <base64.h>           

// --- PIN DEFINITIONS (ESP32-C6) ---
#define GSM_RX_PIN 2   
#define GSM_TX_PIN 7   
#define GPS_RX_PIN 6   
#define DUMMY_TX_PIN 5 

#define PHONE_NUMBER "+916381146811" 
#define GSM_APN "airtelgprs.com" 

class ConnectivityManager {
  private:
    HardwareSerial gsmSerial;
    SoftwareSerial gpsSerial;
    String latestGpsData = "Scanning...";
    
    unsigned long lastWifiCheck = 0;
    unsigned long lastUploadTime = 0; 

    // --- OFFLINE RETRY STORAGE ---
    bool pendingAlert = false;
    unsigned long lastRetryTime = 0;
    String pendingSource = "";
    int pendingSteps = 0;
    int pendingBattery = 0;

  public:
    ConnectivityManager() : gsmSerial(1), gpsSerial(GPS_RX_PIN, DUMMY_TX_PIN) {}

    void begin() {
      if(SERIAL_DEBUG) Serial.println("🌐 Initializing WiFi...");
      WiFi.mode(WIFI_STA); 
      WiFi.begin(WIFI_SSID, WIFI_PASS);

      gsmSerial.begin(9600, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
      gpsSerial.begin(9600);
      
      delay(3000); 
      gsmSerial.println("AT+CMGF=1"); 
      delay(500);
      gsmSerial.println("AT+CLTS=1"); 
      delay(500);

      if(SERIAL_DEBUG) Serial.println("📶 Configuring 4G LTE APN...");
      gsmSerial.print("AT+CGDCONT=1,\"IP\",\"");
      gsmSerial.print(GSM_APN);
      gsmSerial.println("\"");
      delay(1000);
    }
    
    void update() {
      unsigned long now = millis();

      if (now - lastWifiCheck > NETWORK_CHECK_RATE) {
        lastWifiCheck = now;
        if (WiFi.status() != WL_CONNECTED) {
          WiFi.disconnect();
          WiFi.begin(WIFI_SSID, WIFI_PASS); 
        }
      }

      if (now - lastUploadTime > UPLOAD_RATE) {
        lastUploadTime = now;
        sendHeartbeat();
      }

      if (pendingAlert && (now - lastRetryTime > 10000)) {
        if(SERIAL_DEBUG) Serial.println("🔄 Retrying saved SOS Alert...");
        lastRetryTime = now;
        sendPanicAlert(pendingSteps, pendingBattery, pendingSource);
      }

      while (gpsSerial.available()) {
        String line = gpsSerial.readStringUntil('\n');
        if (line.startsWith("$GNRMC") || line.startsWith("$GPRMC")) {
          latestGpsData = line; 
        }
      }
    }

    String getFallbackLBS() {
      if(SERIAL_DEBUG) Serial.println("🛰️ GPS Invalid. Triangulating via 4G Cell Towers...");
      
      gsmSerial.println("AT+CLBS=1,1");
      
      long startWait = millis();
      String response = "";
      
      while(millis() - startWait < 10000) {
        while (gsmSerial.available()) {
          char c = gsmSerial.read();
          response += c;
        }
        if(response.indexOf("OK") != -1 && response.indexOf("+CLBS:") != -1) break;
      }
      
      int locIndex = response.indexOf("+CLBS: ");
      if (locIndex != -1) {
        int firstComma = response.indexOf(',', locIndex);
        int secondComma = response.indexOf(',', firstComma + 1);
        int thirdComma = response.indexOf(',', secondComma + 1);
        
        if (firstComma != -1 && secondComma != -1 && thirdComma != -1) {
          String lon = response.substring(firstComma + 1, secondComma);
          String lat = response.substring(secondComma + 1, thirdComma);
          if(SERIAL_DEBUG) Serial.println("✅ 4G Triangulation Success: " + lat + "," + lon);
          return "https://maps.google.com/?q=" + lat + "," + lon;
        }
      }
      return "Location Unavailable";
    }
      
    // --- 3-TIER SMART FAILOVER ALERTING ---
    void sendPanicAlert(int stepCount, int batteryLevel, String source) {
      
      String finalLocation = latestGpsData;
      if (latestGpsData.indexOf(",V,") > 0 || latestGpsData.indexOf("Scanning") >= 0) {
        finalLocation = getFallbackLBS();
      }

      // TIER 1: PRIMARY (Hardware GSM SMS)
      if (isGsmAvailable()) {
        Serial.println("📱 TIER 1: Attempting Hardware GSM SMS...");
        bool gsmSuccess = sendGsmSms(source, finalLocation);
        if (gsmSuccess) {
           pendingAlert = false; 
           return;
        } else {
           Serial.println("❌ GSM Failed to send. Falling back...");
        }
      } else {
         Serial.println("⚠️ TIER 1: GSM Module Offline/No Signal.");
      }

      // TIER 2: BACKUP (WiFi API SMS + Backend Dashboard)
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("🌐 TIER 2: Attempting WiFi API SMS...");
        bool twilioSuccess = sendTwilioSMS(source, finalLocation);
        
        if (twilioSuccess) {
          // ---> THE FIX: Now it fires to your NestJS Backend too! <---
          postToBackend(stepCount, batteryLevel, source, finalLocation);
          pendingAlert = false; 
          return;
        }
      }

      // TIER 3: OFFLINE (Store Locally + Retry)
      Serial.println("⚠️ TIER 3: Both Networks Down! Storing SOS locally for retry.");
      pendingAlert = true;
      pendingSource = source;
      pendingSteps = stepCount;
      pendingBattery = batteryLevel;
      lastRetryTime = millis();
    }

    bool isGsmAvailable() {
      while(gsmSerial.available()) gsmSerial.read();
      
      gsmSerial.println("AT+CREG?");
      long startWait = millis();
      String response = "";
      while(millis() - startWait < 2000) {
        while(gsmSerial.available()) {
          response += (char)gsmSerial.read();
        }
        if(response.indexOf("OK") != -1) break;
      }
      
      if (response.indexOf("+CREG: 0,1") != -1 || response.indexOf("+CREG: 0,5") != -1) {
        return true;
      }
      return false;
    }

    bool sendGsmSms(String source, String locationPayload) {
      gsmSerial.print("AT+CMGS=\""); gsmSerial.print(PHONE_NUMBER); gsmSerial.println("\"");
      delay(2000); 
      gsmSerial.print("SOS ALERT! Source: "); gsmSerial.print(source);
      gsmSerial.print("\nLocation:\n"); gsmSerial.print(locationPayload); 
      delay(500);
      gsmSerial.write(26); 
      
      long startWait = millis();
      String response = "";
      while(millis() - startWait < 5000) {
        while(gsmSerial.available()) response += (char)gsmSerial.read();
        if(response.indexOf("OK") != -1) {
          Serial.println("✅ GSM SMS Sent!");
          return true;
        }
        if(response.indexOf("ERROR") != -1) break;
      }
      return false; 
    }

    bool sendTwilioSMS(String source, String locationPayload) {
      WiFiClientSecure client;
      client.setInsecure(); 
      HTTPClient http;

      String url = "https://api.twilio.com/2010-04-01/Accounts/" + String(TWILIO_ACCOUNT_SID) + "/Messages.json";
      http.begin(client, url);

      String auth = String(TWILIO_ACCOUNT_SID) + ":" + String(TWILIO_AUTH_TOKEN);
      String authBase64 = base64::encode(auth);
      http.addHeader("Authorization", "Basic " + authBase64);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      String messageBody = "SOS ALERT! Source: " + source + "\nLoc: " + locationPayload;
      String postData = "To=" + String(PHONE_NUMBER) + "&From=" + String(TWILIO_FROM_NUM) + "&Body=" + messageBody;

      int httpResponseCode = http.POST(postData);

      if (httpResponseCode == 201) {
        Serial.println("✅ Twilio API SMS Sent!");
        http.end();
        return true;
      } else {
        Serial.print("❌ Twilio API Error: "); Serial.println(httpResponseCode);
        http.end();
        return false;
      }
    }

    // --- NESTJS BACKEND FUNCTION ---
    void postToBackend(int stepCount, int batteryLevel, String source, String finalLocation) {
        Serial.println("🚀 Attempting Dashboard Backend Request...");
        WiFiClient client;
        HTTPClient http;
        String url = "http://" + String(SERVER_IP) + ":" + String(SERVER_PORT) + String(API_ENDPOINT);
        http.setTimeout(4000); 

        if (http.begin(client, url)) {
           http.addHeader("Content-Type", "application/json");
           String jsonData = "{";
           jsonData += "\"deviceId\": \"Nesso-001\",";
           jsonData += "\"type\": \"PANIC\",";
           jsonData += "\"source\": \"" + source + "\",";
           jsonData += "\"steps\": " + String(stepCount) + ",";
           jsonData += "\"battery\": " + String(batteryLevel) + ",";
           jsonData += "\"location\": \"" + finalLocation + "\"";
           jsonData += "}";

           int code = http.POST(jsonData);
           if (code == 200 || code == 201) {
             Serial.println("✅ Backend Received Alert Successfully!");
           } else {
             Serial.print("❌ Backend Error: "); Serial.println(code);
           }
           http.end();
        } else {
           Serial.println("❌ Could not connect to NestJS Server URL.");
        }
    }

    void sendHeartbeat() {
      if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client;
        HTTPClient http;
        String url = "http://" + String(SERVER_IP) + ":" + String(SERVER_PORT) + "/api/telemetry"; 
        http.setTimeout(3000); 
        if (http.begin(client, url)) {
           http.addHeader("Content-Type", "application/json");
           String jsonData = "{\"deviceId\":\"Nesso-001\",\"batteryLevel\":" + String(Core.batteryLevel) + ",\"batteryVoltage\":" + String(Core.batteryVoltage) + ",\"steps\":" + String(Sensors.stepCount) + ",\"isCharging\":" + String(Core.isCharging ? "true" : "false") + "}";
           int code = http.POST(jsonData);
           if (code == 200 || code == 201) {
             if(SERIAL_DEBUG) Serial.println("✅ Heartbeat Synced to Dashboard!");
           }
           http.end();
        }
      }
    }
    
    void syncTimeWithWiFi() {
      if(SERIAL_DEBUG) Serial.println("🔄 Requesting Network Time from WiFi (NTP)...");
      configTime(19800, 0, "pool.ntp.org", "time.nist.gov"); 
      struct tm timeinfo;
      if(!getLocalTime(&timeinfo, 5000)){
        if(SERIAL_DEBUG) Serial.println("❌ Failed to obtain time from WiFi");
        return;
      }
      Core.setTime(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }

    void syncTimeWithGSM() {
      gsmSerial.println("AT+CCLK?"); 
      long startWait = millis();
      while(millis() - startWait < 2000) {
        if (gsmSerial.available()) {
          String resp = gsmSerial.readStringUntil('\n');
          if (resp.indexOf("+CCLK:") != -1) {
            int commaIndex = resp.indexOf(',');
            if (commaIndex != -1) {
              int h = resp.substring(commaIndex + 1, commaIndex + 3).toInt();
              int m = resp.substring(commaIndex + 4, commaIndex + 6).toInt();
              int s = resp.substring(commaIndex + 7, commaIndex + 9).toInt();
              Core.setTime(h, m, s); 
            }
          }
        }
      }
    }
};

extern ConnectivityManager Connectivity;

#endif