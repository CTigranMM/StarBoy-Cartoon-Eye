# GC9A01A Eye Animation

Standalone code for a dynamic, 60FPS fluid eye animation running on an ESP32 using the Adafruit GC9A01A round LCD display. 
Features 3D perspective distortion, pupil physics (spring-damper system), and squashing/stretching animations.

## Hardware Requirements
- ESP32 (Tested on ESP32-S3)
- GC9A01A Round LCD Display

## Software Dependencies
- [PlatformIO](https://platformio.org/)
- Adafruit GFX Library
- Adafruit GC9A01A Library

## Setup
1. Clone this repository.
2. Open with PlatformIO.
3. Update the pin definitions in `src/main.cpp` according to your wiring:
   ```cpp
   #define TFT_CS   14
   #define TFT_DC   21
   #define TFT_MOSI 38
   #define TFT_SCLK 47
   #define TFT_RST  42
   ```
4. Build and upload to your board.
