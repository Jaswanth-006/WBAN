#ifndef UI_H
#define UI_H

#include <M5GFX.h>
#include <WiFi.h>
#include "Core.h"
#include "Sensors.h"

// Tell UI that these exist in the main sketch
extern void handlePanic(String source);
extern float currentPressure; // Bring in the dress pressure!

class UIManager {
  public:
    M5GFX display; 
    Page currentPage = PAGE_CLOCK;
    
    // Touch tracking variables
    bool wasTouched = false;
    int touchStartX = 0;
    int touchStartY = 0;

    void begin() {
      display.begin();
      display.setRotation(1); // Adjust if your Nesso screen is upside down
      display.fillScreen(TFT_BLACK);
      
      pinMode(LCD_BACKLIGHT, OUTPUT);
      digitalWrite(LCD_BACKLIGHT, HIGH);
      
      showBoot();
    }

    void update() {
      if (!Core.isScreenOn) return;

      // 1. Check for touch input instantly
      handleTouch();

      // 2. Redraw the screen every 200ms
      static unsigned long lastDraw = 0;
      if (millis() - lastDraw > 200) { 
        if (currentPage == PAGE_CLOCK) drawClock();
        else if (currentPage == PAGE_SAFETY_CHECK) drawSafetyCheck();
        else if (currentPage == PAGE_INFO) drawInfo();
        else if (currentPage == PAGE_PANIC) drawPanic();
        else if (currentPage == PAGE_LOW_BATT) drawLowBattery();
        lastDraw = millis();
      }
    }

    // --- TOUCH ENGINE ---
    void handleTouch() {
      uint16_t x, y;
      bool isTouched = display.getTouch(&x, &y);

      if (isTouched) {
        Core.resetScreenTimeout(); // Wake or keep screen on!
        
        if (!wasTouched) {
          // Finger just pressed down
          touchStartX = x;
          touchStartY = y;
          wasTouched = true;
        }
      } else {
        if (wasTouched) {
          // Finger just lifted! Process the tap
          wasTouched = false;
          
          // --- LOGIC FOR SLIDE 2 (SAFETY CHECK) ---
          if (currentPage == PAGE_SAFETY_CHECK) {
             if (touchStartX < 120) { 
                // Tapped Left (YES / SAFE)
                currentPage = PAGE_CLOCK; 
                display.fillScreen(TFT_BLACK);
             } else { 
                // Tapped Right (NO / PANIC)
                handlePanic("TOUCH_SOS"); 
             }
          } 
          // --- LOGIC FOR SLIDES 1 & 3 (NAVIGATION) ---
          else if (currentPage == PAGE_CLOCK || currentPage == PAGE_INFO) {
             if (touchStartX > 180) { 
                // Tapped Right Edge -> Next Page
                nextPage();
             } else if (touchStartX < 60) {
                // Tapped Left Edge -> Prev Page
                prevPage();
             }
          }
        }
      }
    }

    void nextPage() {
        display.fillScreen(TFT_BLACK);
        if (currentPage == PAGE_CLOCK) currentPage = PAGE_SAFETY_CHECK;
        else if (currentPage == PAGE_SAFETY_CHECK) currentPage = PAGE_INFO;
        else if (currentPage == PAGE_INFO) currentPage = PAGE_CLOCK;
    }

    void prevPage() {
        display.fillScreen(TFT_BLACK);
        if (currentPage == PAGE_CLOCK) currentPage = PAGE_INFO;
        else if (currentPage == PAGE_INFO) currentPage = PAGE_SAFETY_CHECK;
        else if (currentPage == PAGE_SAFETY_CHECK) currentPage = PAGE_CLOCK;
    }

    void showBoot() {
      display.setTextDatum(middle_center);
      display.setTextSize(2);
      display.setTextColor(TFT_BLUE);
      display.drawString("NESSO N1", 120, 60);
      delay(1000);
      display.fillScreen(TFT_BLACK);
    }

    // --- SLIDE 1: THE CLOCK ---
    void drawClock() {
      display.setTextDatum(middle_center);
      display.setTextSize(4); 
      display.setTextColor(TFT_WHITE, TFT_BLACK); // Overwrite background to prevent flickering
      
      String timeStr = (Core.hour < 10 ? "0" : "") + String(Core.hour) + ":" + 
                       (Core.minute < 10 ? "0" : "") + String(Core.minute) + ":" +
                       (Core.second < 10 ? "0" : "") + String(Core.second);
      display.drawString(timeStr, 120, 100);

      // Status Text
      display.setTextSize(1);
      if (WiFi.status() == WL_CONNECTED) {
        display.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
        display.drawString("WIFI ONLINE", 120, 140);
        display.fillCircle(220, 20, 6, TFT_GREEN); 
      } else {
        display.setTextColor(TFT_ORANGE, TFT_BLACK);
        display.drawString("WIFI OFFLINE", 120, 140);
        display.fillCircle(220, 20, 6, TFT_RED);   
      }

      // Swipe Indicators
      display.setTextColor(TFT_DARKGREY, TFT_BLACK);
      display.drawString("< INFO", 30, 120);
      display.drawString("SAFE >", 210, 120);
    }

    // --- SLIDE 2: SAFETY CHECK ---
    void drawSafetyCheck() {
      // Draw Split Screen Buttons
      display.fillRect(0, 0, 120, 240, TFT_DARKGREEN); // Left Half
      display.fillRect(120, 0, 120, 240, TFT_MAROON);  // Right Half

      display.setTextDatum(middle_center);
      display.setTextSize(2);
      
      display.setTextColor(TFT_WHITE);
      display.drawString("ARE YOU", 120, 40);
      display.drawString("SAFE?", 120, 70);

      // Button Labels
      display.setTextSize(3);
      display.drawString("YES", 60, 140);
      display.drawString("NO!", 180, 140);
    }

    // --- SLIDE 3: TELEMETRY INFO ---
    void drawInfo() {
      display.setTextDatum(top_left);
      display.setTextSize(2);
      display.setTextColor(TFT_ORANGE, TFT_BLACK);
      display.drawString("SYS INFO", 10, 10);
      
      display.setTextSize(2);
      display.setTextColor(TFT_WHITE, TFT_BLACK);
      
      // Display all requested stats!
      display.setCursor(10, 50);
      display.printf("Steps: %d    \n", Sensors.stepCount);
      display.printf("Batt : %d%%   \n", Core.batteryLevel);
      display.printf("Dress: %.1f  \n", currentPressure); // ESP-NOW Pressure Data!

      if(Core.isCharging) {
        display.setTextColor(TFT_GREEN, TFT_BLACK);
        display.println("Status: CHARGE ");
      } else {
        display.setTextColor(TFT_YELLOW, TFT_BLACK);
        display.println("Status: DISCHG");
      }
      
      // Swipe Indicators
      display.setTextDatum(middle_center);
      display.setTextSize(1);
      display.setTextColor(TFT_DARKGREY, TFT_BLACK);
      display.drawString("< SAFE", 30, 120);
      display.drawString("CLOCK >", 210, 120);
    }

    // --- ALERT SCREENS ---
    void drawLowBattery() {
      display.fillScreen(TFT_RED);
      display.setTextDatum(middle_center);
      display.setTextColor(TFT_WHITE);
      display.setTextSize(3);
      display.drawString("LOW BATT", 120, 90);
    }

    void drawPanic() {
      bool flash = (millis() / 300) % 2;
      display.fillScreen(flash ? TFT_RED : TFT_BLACK);
      display.setTextColor(flash ? TFT_WHITE : TFT_RED);
      display.setTextDatum(middle_center);
      display.setTextSize(4);
      display.drawString("SOS", 120, 120);
    }
};

extern UIManager UI;

#endif