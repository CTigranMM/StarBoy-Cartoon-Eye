# PocketBuddy Eyes

**Hardware Demo:**
[👉 Click here to watch the Hardware Demo Video](https://github.com/CTigranMM/StarBoy-Cartoon-Eye/raw/main/Video.mp4)

**Generated Preview:**
![Eye Animation Preview](preview.gif)

Fluid, 3D animated eyes for the GC9A01A round LCD display. 

This library was extracted from a larger personal project (Pocket-Buddy) to make the eye animation code open-source, resulting in a cleaner, easily reusable codebase. The visual style and concept are heavily inspired by the Starboy keychain product (https://hesjustalittleguy.com/).

It packages a 60FPS physics-based eye animation system into a simple C++ class.

## Features
- Dynamic 60FPS animation system
- Hooke's Law spring-damper physics for natural, bouncy movements
- 3D perspective distortion during movement
- Automatic blinking, squashing, and stretching behaviors
- Mathematical mask generation (no external image assets required)

## Hardware Requirements
- Microcontroller with sufficient RAM and speed (Tested on ESP32-S3)
- GC9A01A Round LCD Display (240x240 pixels)

## Installation

### PlatformIO
Add the library to your `platformio.ini` file:
```ini
lib_deps =
    adafruit/Adafruit GFX Library
    adafruit/Adafruit GC9A01A
    https://github.com/YOUR_USERNAME/YOUR_REPO_NAME.git
```

### Arduino IDE
Download this repository as a `.zip` file and install it via Sketch > Include Library > Add .ZIP Library...

## Usage

Check out the `examples/BasicAnimation` folder for a complete runnable example. Here is a quick snippet:

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
    // Handles physics, framerate, and drawing automatically
    eyes.update(); 
}
```

## Use of AI
This codebase was extracted from the original Pocket-Buddy project and refactored into a clean, standalone C++ library with the assistance of AI.

## Keywords
*Starboy keychain, cartoon eyes, GC9A01A, round LCD, digital companion, robot eyes, expressive eyes, physics animation, ESP32 display, open source starboy, little guy keychain.*
