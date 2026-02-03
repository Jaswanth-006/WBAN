#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- DEBUG SETTINGS ---
#define SERIAL_DEBUG    true
#define SCREEN_TIMEOUT  10000 
#define STEP_THRESHOLD  1.3    

// --- WIFI SETTINGS (For Testing without GSM) ---
#define WIFI_SSID       "Jass"      // <--- CHANGE THIS
#define WIFI_PASS       "godlvlgodlvl"  // <--- CHANGE THIS



// --- WIFI SETTINGS ---
#define WIFI_SSID       "Jass" 
#define WIFI_PASS       "godlvlgodlvl"

// --- BACKEND CONNECTION ---
#define SERVER_IP       "10.200.18.42"      // <--- UPDATED WITH YOUR REAL IP
#define SERVER_PORT     3000
#define API_ENDPOINT    "/api/alert"         // Keep this (matches your backend log)

// --- SYSTEM STATES ---
enum SystemState { STATE_IDLE, STATE_ACTIVE, STATE_PANIC };
enum Page { PAGE_CLOCK, PAGE_INFO, PAGE_PANIC };

#endif