# PocketBuddy Eyes

Fluid, 3D animated eyes for the GC9A01A round LCD display. 

This library packages up a 60FPS physics-based eye animation system (with 3D perspective distortion, squashing, blinking, and Hooke's Law spring-damper physics) into an easy-to-use C++ class.

## Hardware Requirements
- ESP32 (Tested on ESP32-S3)
- GC9A01A Round LCD Display

## Installation
### PlatformIO
Add the library to your `platformio.ini`:
```ini
lib_deps =
    adafruit/Adafruit GFX Library
    adafruit/Adafruit GC9A01A
    https://github.com/YOUR_USERNAME/YOUR_REPO_NAME.git
```

### Arduino IDE
Download this repository as a `.zip` and install it via **Sketch > Include Library > Add .ZIP Library...**

## Usage

Check out the [`examples/BasicAnimation`](examples/BasicAnimation) folder for a complete runnable example. Here is a quick snippet:

```cpp
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <PocketBuddyEyes.h>

// Set your SPI pins here
#define TFT_CS   14
#define TFT_DC   21
#define TFT_MOSI 38
#define TFT_SCLK 47
#define TFT_RST  42

Adafruit_GC9A01A tft = Adafruit_GC9A01A(&SPI, TFT_DC, TFT_CS, TFT_RST);
PocketBuddyEyes eyes(&tft);

void setup() {
    SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
    tft.begin(40000000);
    tft.fillScreen(0x0000);
    
    // Optional: Change the colors
    // eyes.setPupilColor(tft.color565(67, 120, 98)); 

    eyes.begin();
}

void loop() {
    eyes.update(); // Handles physics, framerate, and drawing automatically
}
```
