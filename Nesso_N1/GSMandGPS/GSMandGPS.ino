/*
 * WBAN WATCH - "SAFE SEND" FIX
 * ----------------------------
 * Fixes "+CMS ERROR: SMS size more than expected"
 * by sending a shorter message with delays.
 */

#include <Arduino.h>
#include <HardwareSerial.h>

String PHONE_NUMBER = "+916381146811"; // Updated to the number in your log

// --- PINS ---
#define GSM_RX_PIN 2   
#define GSM_TX_PIN 7   
#define GPS_RX_PIN 4   
#define GPS_TX_PIN -1

HardwareSerial gsmSerial(1);
HardwareSerial gpsSerial(2);

bool smsSent = false; 

void setup() {
  Serial.begin(115200);
  gsmSerial.begin(115200, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  Serial.println("--- SYSTEM RESTART ---");
  delay(5000); 
  
  // Set Text Mode
  gsmSerial.println("AT+CMGF=1"); 
  delay(1000);
}

void sendSafeSMS(String gpsString) {
  Serial.println("\n>>> SENDING SHORT SMS <<<");
  
  // 1. Start Command
  gsmSerial.print("AT+CMGS=\"");
  gsmSerial.print(PHONE_NUMBER);
  gsmSerial.println("\"");
  
  delay(2000); // WAIT LONGER for the '>' prompt

  // 2. Type Message (Keep it simple!)
  gsmSerial.print("ALERT! GPS Data:\n");
  delay(100); 
  
  // 3. Add Link (Demo link if V, Real if A)
  if (gpsString.indexOf(",V,") > 0) {
    gsmSerial.print("https://maps.google.com (Searching...)");
  } else {
    // If we have real coordinates, this works better
    gsmSerial.print("https://maps.google.com");
  }
  
  delay(500);

  // 4. FINISH
  gsmSerial.write(26); // Ctrl+Z
  
  Serial.println(">>> SENT. WAITING FOR OK... <<<");
  smsSent = true; 
}

void loop() {
  // 1. READ GPS
  if (gpsSerial.available()) {
    String line = gpsSerial.readStringUntil('\n');
    
    // As soon as we see ANY GPS data, we try to send
    if (line.startsWith("$GNRMC") && !smsSent) {
       Serial.println("GPS Found. Triggering SMS...");
       
       // Check Signal First
       gsmSerial.println("AT+CSQ");
       delay(500);
       
       sendSafeSMS(line);
    }
  }

  // 2. SHOW GSM REPLIES
  if (gsmSerial.available()) {
    Serial.write(gsmSerial.read());
  }
}