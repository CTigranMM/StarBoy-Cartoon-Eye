#ifndef POCKET_BUDDY_EYES_H
#define POCKET_BUDDY_EYES_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>

struct PB_Spring {
    float pos = 0;
    float vel = 0;
    float target = 0;
    float k = 180.0f; // Stiffness (higher = faster snap)
    float d = 12.0f;  // Damping (lower = more bouncy wobble)
    
    void update(float dt) {
        float force = -k * (pos - target) - d * vel;
        vel += force * dt;
        pos += vel * dt;
    }
};

enum PB_Emotion { PB_IDLE, PB_SQUASH_ANTICIPATION };

class PocketBuddyEyes {
public:
    PocketBuddyEyes(Adafruit_GC9A01A* tftDisplay);
    
    void begin();
    void update(); // Call this continuously in loop()

    // Optional style controls
    void setPupilColor(uint16_t color);
    void setBgColor(uint16_t color);
    void setWhiteColor(uint16_t color);

private:
    Adafruit_GC9A01A* tft;
    
    uint16_t colorBg;
    uint16_t colorWhite;
    uint16_t colorPupil;

    PB_Spring squash;     
    PB_Spring blinkLid;   

    long lastUpdate;
    long nextActionTime;

    PB_Emotion currentEmotion;

    float yaw;
    float targetYaw;
    float pitch;
    float targetPitch;
    float prevYaw;
    float smearX;

    static const int EYE_WIDTH = 100;
    static const int EYE_HEIGHT = 86;
    static const int IRIS_WIDTH = 42;
    static const int IRIS_HEIGHT = 86;
    static const int CHUNK_LINES = 40;

    uint8_t white_eye_mask[EYE_HEIGHT][EYE_WIDTH];
    uint8_t iris_mask[IRIS_HEIGHT][IRIS_WIDTH];
    uint16_t chunkBuffer[240 * CHUNK_LINES];

    void generateMasks();
    void blurMask(uint8_t* mask, int w, int h);
    uint16_t blend565(uint16_t bg, uint16_t fg, uint8_t alpha);
    void drawEyes();
};

#endif
