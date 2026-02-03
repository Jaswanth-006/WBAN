#ifndef UI_H
#define UI_H

#include <M5GFX.h>
#include "Core.h"
#include "Sensors.h"

class UIManager {
  public:
    M5GFX display; // Public for sleep access
    Page currentPage = PAGE_CLOCK;

    void begin() {
      display.begin();
      display.setRotation(1);
      display.fillScreen(TFT_BLACK);
      
      // Force Backlight ON
      pinMode(LCD_BACKLIGHT, OUTPUT);
      digitalWrite(LCD_BACKLIGHT, HIGH);
      
      showBoot();
    }

    void update() {
      if (!Core.isScreenOn) return;

      static unsigned long lastDraw = 0;
      if (millis() - lastDraw > 200) { 
        if (currentPage == PAGE_CLOCK) drawClock();
        else if (currentPage == PAGE_INFO) drawInfo();
        else if (currentPage == PAGE_PANIC) drawPanic();
        lastDraw = millis();
      }
    }

    void showBoot() {
      display.setTextDatum(middle_center);
      display.setTextSize(2);
      display.setTextColor(TFT_BLUE);
      display.drawString("NESSO N1", 120, 60);
      delay(1000);
      display.fillScreen(TFT_BLACK);
    }

    // --- DRAWS ---
    void drawClock() {
      display.fillScreen(TFT_BLACK);
      
      // Time
      display.setTextDatum(middle_center);
      display.setTextSize(5);
      display.setTextColor(TFT_WHITE);
      String timeStr = (Core.hour < 10 ? "0" : "") + String(Core.hour) + ":" + 
                       (Core.minute < 10 ? "0" : "") + String(Core.minute);
      display.drawString(timeStr, 120, 70);

      // Status
      display.setTextSize(1);
      display.setTextColor(TFT_GREENYELLOW);
      display.drawString("SYSTEM ACTIVE", 120, 110);
      
      // WiFi Dot
      display.fillCircle(230, 10, 4, TFT_BLUE);
    }

    void drawInfo() {
      display.fillScreen(TFT_BLACK);
      display.setTextDatum(top_left);
      display.setTextSize(2);
      display.setTextColor(TFT_ORANGE);
      display.drawString("INFO", 10, 10);
      
      display.setTextSize(1);
      display.setTextColor(TFT_WHITE);
      display.setCursor(10, 40);
      display.print("Steps: "); display.println(Sensors.stepCount);
      display.print("Batt: "); display.print(Core.batteryLevel); display.println("%");
    }

    void drawPanic() {
      bool flash = (millis() / 300) % 2;
      display.fillScreen(flash ? TFT_RED : TFT_BLACK);
      display.setTextColor(flash ? TFT_WHITE : TFT_RED);
      display.setTextDatum(middle_center);
      display.setTextSize(3);
      display.drawString("SOS", 120, 70);
    }
};

extern UIManager UI;

#endif