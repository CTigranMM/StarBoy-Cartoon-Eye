#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>

#define TFT_CS   14
#define TFT_DC   21
#define TFT_MOSI 38
#define TFT_SCLK 47
#define TFT_RST  42

Adafruit_GC9A01A tft = Adafruit_GC9A01A(&SPI, TFT_DC, TFT_CS, TFT_RST);

// Colors based on "acorn limeade"
#define COLOR_BG      tft.color565(0, 0, 0) // Very dark background (nearly black)
#define COLOR_WHITE   tft.color565(253, 249, 249)
#define COLOR_PUPIL   tft.color565(67, 120, 98) // Acorn Limeade Green

// 1D Hooke's Law Spring-Damper for fluid, bouncy movement
struct Spring {
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

// Physics States
Spring pupilX, pupilY; 
Spring squash;     // 1.0 = normal, < 1.0 = squashed
Spring blinkLid;   // 0 = open, 1.0 = closed

// Animation timing
long lastUpdate = 0;
long nextActionTime = 0;

const int EYE_WIDTH = 100;
const int EYE_HEIGHT = 86;
const int IRIS_WIDTH = 42;
const int IRIS_HEIGHT = 86;

enum Emotion { IDLE, SQUASH_ANTICIPATION };
Emotion currentEmotion = IDLE;

uint8_t white_eye_mask[EYE_HEIGHT][EYE_WIDTH] = {0};
uint8_t iris_mask[IRIS_HEIGHT][IRIS_WIDTH] = {0};

#define CHUNK_LINES 40
uint16_t chunkBuffer[240 * CHUNK_LINES];


uint16_t blend565(uint16_t bg, uint16_t fg, uint8_t alpha) {
    if (alpha == 255) return fg;
    if (alpha == 0) return bg;
    
    uint32_t a = alpha;
    uint32_t inv_a = 255 - a;
    
    uint32_t br = (bg >> 11) & 0x1F;
    uint32_t bg_g = (bg >> 5) & 0x3F;
    uint32_t bb = bg & 0x1F;
    
    uint32_t fr = (fg >> 11) & 0x1F;
    uint32_t fg_g = (fg >> 5) & 0x3F;
    uint32_t fb = fg & 0x1F;
    
    uint32_t r = (br * inv_a + fr * a) >> 8; // approx /255
    uint32_t g = (bg_g * inv_a + fg_g * a) >> 8;
    uint32_t b = (bb * inv_a + fb * a) >> 8;
    
    return (r << 11) | (g << 5) | b;
}

void blurMask(uint8_t* mask, int w, int h) {
    uint8_t* temp = (uint8_t*)malloc(w * h);
    if(!temp) return;
    
    for(int y=1; y<h-1; y++) {
        for(int x=1; x<w-1; x++) {
            int sum = 
                mask[(y-1)*w + (x-1)] + mask[(y-1)*w + x] + mask[(y-1)*w + (x+1)] +
                mask[y*w + (x-1)]     + mask[y*w + x]     + mask[y*w + (x+1)] +
                mask[(y+1)*w + (x-1)] + mask[(y+1)*w + x] + mask[(y+1)*w + (x+1)];
            temp[y*w + x] = sum / 9;
        }
    }
    
    for(int y=1; y<h-1; y++) {
        for(int x=1; x<w-1; x++) {
            mask[y*w + x] = temp[y*w + x];
        }
    }
    free(temp);
}

void generateMasks() {
    float scale = 0.35f;
    float offsetX = -110.0f; 
    float offsetY = -123.0f; 

    float pts[13][2] = {
        {128.963, 246.454}, 
        {12.4624, 246.454}, {-53.5376, 66.4541}, {57.463, 11.4541}, 
        {83.3209, -1.11654}, {98.8257, -2.98446}, {128.963, 6.45405}, 
        {184.285, 29.9704}, {227.569, 89.648}, {216.962, 160.454}, 
        {212.361, 205.443}, {176.352, 241.615}, {128.963, 246.454} 
    };

    for(int i=0; i<13; i++) {
        pts[i][0] = (pts[i][0] + offsetX) * scale + 57.0f;
        pts[i][1] = (pts[i][1] + offsetY) * scale + 42.0f;
    }

    for(int c=0; c<4; c++) {
        int i = c * 3;
        for(float t=0; t<=1.0f; t+=0.001f) {
            float u = 1.0f - t;
            float px = u*u*u*pts[i][0] + 3*u*u*t*pts[i+1][0] + 3*u*t*t*pts[i+2][0] + t*t*t*pts[i+3][0];
            float py = u*u*u*pts[i][1] + 3*u*u*t*pts[i+1][1] + 3*u*t*t*pts[i+2][1] + t*t*t*pts[i+3][1];
            int ix = round(px);
            int iy = round(py);
            if(ix>=0 && ix<EYE_WIDTH && iy>=0 && iy<EYE_HEIGHT) {
                white_eye_mask[iy][ix] = 255;
            }
        }
    }

    for(int y=0; y<EYE_HEIGHT; y++) {
        int firstX = -1;
        int lastX = -1;
        for(int x=0; x<EYE_WIDTH; x++) {
            if(white_eye_mask[y][x]) {
                if(firstX == -1) firstX = x;
                lastX = x;
            }
        }
        if(firstX != -1 && lastX != -1) {
            for(int x=firstX; x<=lastX; x++) {
                white_eye_mask[y][x] = 255;
            }
        }
    }

    float rot = 3.65f * PI / 180.0f;
    float rx = 56.5f * scale;
    float ry = 118.4f * scale;
    
    for(int y=0; y<IRIS_HEIGHT; y++) {
        for(int x=0; x<IRIS_WIDTH; x++) {
            float dx = x - (IRIS_WIDTH / 2.0f);
            float dy = y - (IRIS_HEIGHT / 2.0f);
            
            float nx = dx * cos(-rot) - dy * sin(-rot);
            float ny = dx * sin(-rot) + dy * cos(-rot);
            
            if ((nx*nx)/(rx*rx) + (ny*ny)/(ry*ry) <= 1.0f) {
                iris_mask[y][x] = 255;
            }
        }
    }
    
    blurMask((uint8_t*)white_eye_mask, EYE_WIDTH, EYE_HEIGHT);
    blurMask((uint8_t*)iris_mask, IRIS_WIDTH, IRIS_HEIGHT);
}
float yaw = 0.0f;
float targetYaw = 0.0f;
float pitch = 0.0f;
float targetPitch = 0.0f;
float prevYaw = 0.0f;
float smearX = 1.0f;

void drawEyes() {
    float sinY = sin(yaw);
    float cosY = cos(yaw);
    float sinP = sin(pitch);
    float cosP = cos(pitch);

    int blinkY = blinkLid.pos * 240.0f;

    for (int startY = 0; startY < 240; startY += CHUNK_LINES) {
        for (int cy = 0; cy < CHUNK_LINES; cy++) {
            int y = startY + cy;
            float v = (y - 120) / 120.0f;
            float v2 = v * v;

            if (y < blinkY) {
                for (int x = 0; x < 240; x++) chunkBuffer[cy * 240 + x] = COLOR_BG;
                continue;
            }
            
            for (int x = 0; x < 240; x++) {
            float u = (x - 120) / 120.0f;
            float r2 = u * u + v2;
            
            if (r2 <= 1.0f) {
                float z = sqrtf(1.0f - r2);
                
                float rx = u * cosY - z * sinY;
                float rz = u * sinY + z * cosY;
                float ry = v * cosP - rz * sinP;
                
                float sY = squash.pos;
                float sX = squash.pos / smearX; 
                if (sY < 0.1f) sY = 0.1f;
                if (sX < 0.1f) sX = 0.1f;
                
                int tx = (int)((rx * sX) * 120.0f + 120.0f);
                int ty = (int)((ry / sY) * 120.0f + 120.0f);

                uint16_t color = COLOR_BG; 

                if (tx >= 0 && tx < 240 && ty >= 0 && ty < 240) {
                    // --- LEFT EYE ---
                    int l_startX = 17;
                    int l_startY = 78;
                    if (tx >= l_startX && tx < l_startX + EYE_WIDTH && ty >= l_startY && ty < l_startY + EYE_HEIGHT) {
                        int lx = tx - l_startX;
                        int ly = ty - l_startY;
                        uint8_t alpha = white_eye_mask[ly][lx];
                        if (alpha > 0) {
                            uint16_t targetColor = COLOR_WHITE;
                            
                            int pBaseX = 47;
                            int pBaseY = 10;
                            int pX = lx - pBaseX;
                            int pY = ly - pBaseY;
                            
                            if (pX >= 0 && pX < IRIS_WIDTH && pY >= 0 && pY < IRIS_HEIGHT) {
                                uint8_t pAlpha = iris_mask[pY][pX];
                                if (pAlpha > 0) {
                                    targetColor = blend565(COLOR_WHITE, COLOR_PUPIL, pAlpha);
                                }
                            }
                            
                            color = blend565(color, targetColor, alpha);
                        }
                    }
                    
                    // --- RIGHT EYE ---
                    int r_startX = 123;
                    int r_startY = 78;
                    if (tx >= r_startX && tx < r_startX + EYE_WIDTH && ty >= r_startY && ty < r_startY + EYE_HEIGHT) {
                        int rx_eye = tx - r_startX;
                        int ry_eye = ty - r_startY;
                        int sourceX = EYE_WIDTH - 1 - rx_eye; // Mirrored reading!
                        
                        uint8_t alpha = white_eye_mask[ry_eye][sourceX];
                        if (alpha > 0) {
                            uint16_t targetColor = COLOR_WHITE;
                            
                            int pBaseX = 11; 
                            int pBaseY = 10;
                            int pX = rx_eye - pBaseX;
                            int pY = ry_eye - pBaseY;
                            int pSourceX = IRIS_WIDTH - 1 - pX; 
                            
                            if (pX >= 0 && pX < IRIS_WIDTH && pY >= 0 && pY < IRIS_HEIGHT) {
                                uint8_t pAlpha = iris_mask[pY][pSourceX];
                                if (pAlpha > 0) {
                                    targetColor = blend565(COLOR_WHITE, COLOR_PUPIL, pAlpha);
                                }
                            }
                            
                            color = blend565(color, targetColor, alpha);
                        }
                    }
                }
                chunkBuffer[cy * 240 + x] = color;
            } else {
                chunkBuffer[cy * 240 + x] = COLOR_BG;
            }
        }
    }
    tft.drawRGBBitmap(0, startY, chunkBuffer, 240, CHUNK_LINES);
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000); 
    
    Serial.println("Generating SVG masks mathematically...");
    generateMasks();
    Serial.println("Masks generated!");
    
    SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
    
    tft.begin(40000000); 
    tft.setRotation(0);
    tft.fillScreen(COLOR_BG); 
    
    squash.pos = 1.0f; squash.target = 1.0f;
    blinkLid.pos = 0; blinkLid.target = 0;
    
    drawEyes();

    Serial.println("\n=== EYE ANIMATION STARTED ===");
}

void loop() {
    long now = millis();
    float dt = (now - lastUpdate) / 1000.0f;
    
    if (dt >= 0.016f) { // ~60 fps update rate
        lastUpdate = now;
        
        if (dt > 0.1f) dt = 0.1f; 

        float remain = dt;
        while (remain > 0.0f) {
            float step = (remain > 0.016f) ? 0.016f : remain;
            squash.update(step);
            blinkLid.update(step);
            yaw += (targetYaw - yaw) * 6.0f * step;
            pitch += (targetPitch - pitch) * 6.0f * step;
            
            float yawVel = abs(yaw - prevYaw) / step;
            smearX = 1.0f + (yawVel * 0.2f); // Smear multiplier
            prevYaw = yaw;
            
            remain -= step;
        }
        
        if (now > nextActionTime) {
            int action = random(100);
            
            if (currentEmotion == SQUASH_ANTICIPATION) {
                squash.target = 1.0f;
                targetYaw = random(-100, 100) / 100.0f * 0.8f;
                targetPitch = random(-100, 100) / 100.0f * 0.4f;
                squash.vel += 15.0f; 
                
                currentEmotion = IDLE;
                nextActionTime = now + random(500, 2000);
            } 
            else if (blinkLid.target > 0.5f) {
                blinkLid.target = 0;
                currentEmotion = IDLE;
                nextActionTime = now + random(1000, 3000);
            } 
            else {
                if (action < 15) {
                    squash.target = 0.7f; 
                    currentEmotion = SQUASH_ANTICIPATION;
                    nextActionTime = now + 150; 
                } else if (action < 40) {
                    blinkLid.target = 1.0f; 
                    currentEmotion = IDLE;
                    nextActionTime = now + 120; 
                } else {
                    targetYaw = random(-100, 100) / 100.0f * 0.8f;
                    targetPitch = random(-100, 100) / 100.0f * 0.4f;
                    currentEmotion = IDLE;
                    nextActionTime = now + random(500, 2000);
                }
            }
        }
        
        drawEyes();
    }
}
