#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#if ENABLE_DISPLAY

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include "config.h"
#include "pid_handler.h"

// ESP32-S3 Feather TFT pins
#define TFT_CS         7
#define TFT_RST        40
#define TFT_DC         39
#define TFT_BACKLIGHT  45

class DisplayManager {
private:
    Adafruit_ST7789* tft;
    PIDHandler* pidHandler;
    unsigned long lastUpdate;
    uint16_t updateInterval;

    // Colors (RGB565)
    const uint16_t COLOR_BG = 0x0000;       // Black
    const uint16_t COLOR_TEXT = 0xFFFF;     // White
    const uint16_t COLOR_PRIMARY = 0xFD20;  // Orange (MockStang)
    const uint16_t COLOR_SUCCESS = 0x07E0;  // Green
    const uint16_t COLOR_WARNING = 0xFFE0;  // Yellow
    const uint16_t COLOR_ERROR = 0xF800;    // Red

public:
    DisplayManager(PIDHandler* handler)
        : pidHandler(handler), lastUpdate(0), updateInterval(100) {
        tft = new Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
    }

    void begin() {
        Serial.println("Initializing display...");

        // Turn on backlight
        pinMode(TFT_BACKLIGHT, OUTPUT);
        digitalWrite(TFT_BACKLIGHT, HIGH);

        // Initialize display
        tft->init(DISPLAY_HEIGHT, DISPLAY_WIDTH);
        tft->setRotation(DISPLAY_ROTATION);
        tft->fillScreen(COLOR_BG);

        Serial.println("Display initialized");
    }

    void showSplash() {
        tft->fillScreen(COLOR_BG);
        tft->setTextColor(COLOR_PRIMARY);
        tft->setTextSize(3);
        tft->setCursor(20, 40);
        tft->println("MockStang");

        tft->setTextColor(COLOR_TEXT);
        tft->setTextSize(1);
        tft->setCursor(20, 80);
        tft->println("WiFi OBD-II Emulator");
        tft->setCursor(20, 95);
        tft->print("Platform: ");
        tft->println(PLATFORM_NAME);

        delay(2000);
    }

    void update() {
        unsigned long now = millis();
        if (now - lastUpdate < updateInterval) {
            return;
        }
        lastUpdate = now;

        // Get current state
        CarState state = pidHandler->getState();

        // Clear screen
        tft->fillScreen(COLOR_BG);

        // Title
        tft->setTextColor(COLOR_PRIMARY);
        tft->setTextSize(2);
        tft->setCursor(10, 5);
        tft->println("MockStang");

        // Connection status
        drawConnectionStatus(10, 30);

        // Vehicle parameters (2 columns)
        drawParameter(10, 50, "RPM:", state.rpm, "");
        drawParameter(130, 50, "Speed:", state.speed, "km/h");

        drawParameter(10, 70, "Coolant:", state.coolant_temp, "C");
        drawParameter(130, 70, "Intake:", state.intake_temp, "C");

        drawParameter(10, 90, "Throttle:", state.throttle, "%");
        drawParameter(130, 90, "Fuel:", state.fuel_level, "%");

        // MIL status
        if (state.mil_on) {
            drawMIL(10, 115);
        }
    }

    void drawConnectionStatus(int x, int y) {
        tft->setTextSize(1);
        tft->setCursor(x, y);
        tft->setTextColor(COLOR_TEXT);
        tft->print("WiFi: ");
        tft->setTextColor(COLOR_SUCCESS);
        tft->println("Active");

        // TODO: Add BLE status
        #if ENABLE_BLE
        tft->setCursor(x + 100, y);
        tft->setTextColor(COLOR_TEXT);
        tft->print("BLE: ");
        tft->setTextColor(COLOR_SUCCESS);
        tft->println("Ready");
        #endif
    }

    void drawParameter(int x, int y, const char* label, int value, const char* unit) {
        tft->setTextSize(1);
        tft->setTextColor(COLOR_TEXT);
        tft->setCursor(x, y);
        tft->print(label);

        tft->setCursor(x, y + 10);
        tft->setTextColor(COLOR_PRIMARY);
        tft->print(value);
        tft->print(" ");
        tft->print(unit);
    }

    void drawMIL(int x, int y) {
        tft->fillRect(x, y, 60, 15, COLOR_ERROR);
        tft->setTextColor(COLOR_TEXT);
        tft->setTextSize(1);
        tft->setCursor(x + 5, y + 4);
        tft->print("CHECK ENG");
    }

    void showMessage(const char* message, uint16_t color = 0xFFFF) {
        tft->fillRect(0, 100, DISPLAY_WIDTH, 30, COLOR_BG);
        tft->setTextColor(color);
        tft->setTextSize(1);
        tft->setCursor(10, 110);
        tft->println(message);
    }

    void setBrightness(uint8_t level) {
        // Simple on/off for now
        digitalWrite(TFT_BACKLIGHT, level > 0 ? HIGH : LOW);
    }
};

#endif // ENABLE_DISPLAY

#endif // DISPLAY_MANAGER_H
