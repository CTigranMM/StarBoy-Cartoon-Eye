#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <PocketBuddyEyes.h>

// Update these pins to match your hardware!
#define TFT_CS   14
#define TFT_DC   21
#define TFT_MOSI 38
#define TFT_SCLK 47
#define TFT_RST  42

Adafruit_GC9A01A tft = Adafruit_GC9A01A(&SPI, TFT_DC, TFT_CS, TFT_RST);
PocketBuddyEyes eyes(&tft);

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    // Initialize SPI and TFT
    SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
    tft.begin(40000000); // 40MHz for smooth rendering
    tft.setRotation(0);
    tft.fillScreen(0x0000); // Black background
    
    Serial.println("Initializing Eyes...");
    
    // Optional: Customize eye colors
    // eyes.setPupilColor(tft.color565(0, 0, 255)); // Blue eyes!
    
    // Begin calculates masks and starts physics
    eyes.begin();
    
    Serial.println("Eyes Started!");
}

void loop() {
    // Automatically handles the 60fps draw updates and random idle animations
    eyes.update();
    
    // Yield to let ESP32 handle background tasks
    delay(1);
}
