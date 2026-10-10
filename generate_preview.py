import numpy as np
import cv2
import math
import subprocess
import os

width = 240
height = 240
EYE_WIDTH = 100
EYE_HEIGHT = 86
IRIS_WIDTH = 42
IRIS_HEIGHT = 86

white_eye_mask = np.zeros((EYE_HEIGHT, EYE_WIDTH), dtype=np.uint8)
iris_mask = np.zeros((IRIS_HEIGHT, IRIS_WIDTH), dtype=np.uint8)

def generate_masks():
    scale = 0.35
    offsetX = -110.0
    offsetY = -123.0
    
    pts = [
        [128.963, 246.454], 
        [12.4624, 246.454], [-53.5376, 66.4541], [57.463, 11.4541], 
        [83.3209, -1.11654], [98.8257, -2.98446], [128.963, 6.45405], 
        [184.285, 29.9704], [227.569, 89.648], [216.962, 160.454], 
        [212.361, 205.443], [176.352, 241.615], [128.963, 246.454] 
    ]
    
    for i in range(13):
        pts[i][0] = (pts[i][0] + offsetX) * scale + 57.0
        pts[i][1] = (pts[i][1] + offsetY) * scale + 42.0

    for c in range(4):
        i = c * 3
        for t_int in range(1001):
            t = t_int / 1000.0
            u = 1.0 - t
            px = u*u*u*pts[i][0] + 3*u*u*t*pts[i+1][0] + 3*u*t*t*pts[i+2][0] + t*t*t*pts[i+3][0]
            py = u*u*u*pts[i][1] + 3*u*u*t*pts[i+1][1] + 3*u*t*t*pts[i+2][1] + t*t*t*pts[i+3][1]
            ix = int(round(px))
            iy = int(round(py))
            if 0 <= ix < EYE_WIDTH and 0 <= iy < EYE_HEIGHT:
                white_eye_mask[iy, ix] = 255

    for y in range(EYE_HEIGHT):
        indices = np.where(white_eye_mask[y] == 255)[0]
        if len(indices) > 0:
            firstX = indices[0]
            lastX = indices[-1]
            white_eye_mask[y, firstX:lastX+1] = 255

    rot = 3.65 * math.pi / 180.0
    rx = 56.5 * scale
    ry = 118.4 * scale
    
    for y in range(IRIS_HEIGHT):
        for x in range(IRIS_WIDTH):
            dx = x - (IRIS_WIDTH / 2.0)
            dy = y - (IRIS_HEIGHT / 2.0)
            nx = dx * math.cos(-rot) - dy * math.sin(-rot)
            ny = dx * math.sin(-rot) + dy * math.cos(-rot)
            if (nx*nx)/(rx*rx) + (ny*ny)/(ry*ry) <= 1.0:
                iris_mask[y, x] = 255
                
def blur_mask(mask):
    temp = np.zeros_like(mask)
    h, w = mask.shape
    for y in range(1, h-1):
        for x in range(1, w-1):
            s = int(mask[y-1,x-1]) + int(mask[y-1,x]) + int(mask[y-1,x+1]) + \
                int(mask[y,x-1]) + int(mask[y,x]) + int(mask[y,x+1]) + \
                int(mask[y+1,x-1]) + int(mask[y+1,x]) + int(mask[y+1,x+1])
            temp[y,x] = s // 9
    mask[1:h-1, 1:w-1] = temp[1:h-1, 1:w-1]

generate_masks()
blur_mask(white_eye_mask)
blur_mask(iris_mask)

# BGR colors for cv2
COLOR_BG = np.array([0, 0, 0], dtype=np.float32)
COLOR_WHITE = np.array([249, 249, 253], dtype=np.float32) 
COLOR_PUPIL = np.array([62, 120, 67], dtype=np.float32)

print("Pre-rendering static texture mapping...")
texture = np.zeros((240, 240, 3), dtype=np.float32)

for ty in range(240):
    for tx in range(240):
        color = COLOR_BG.copy()
        
        l_startX = 17
        l_startY = 78
        if l_startX <= tx < l_startX + EYE_WIDTH and l_startY <= ty < l_startY + EYE_HEIGHT:
            lx = tx - l_startX
            ly = ty - l_startY
            alpha = white_eye_mask[ly, lx]
            if alpha > 0:
                targetColor = COLOR_WHITE.copy()
                pX = lx - 47
                pY = ly - 10
                if 0 <= pX < IRIS_WIDTH and 0 <= pY < IRIS_HEIGHT:
                    pAlpha = iris_mask[pY, pX]
                    if pAlpha > 0:
                        a2 = pAlpha / 255.0
                        targetColor = COLOR_WHITE * (1.0-a2) + COLOR_PUPIL * a2
                
                a = alpha / 255.0
                color = color * (1.0-a) + targetColor * a
                
        r_startX = 123
        r_startY = 78
        if r_startX <= tx < r_startX + EYE_WIDTH and r_startY <= ty < r_startY + EYE_HEIGHT:
            rx_eye = tx - r_startX
            ry_eye = ty - r_startY
            sourceX = EYE_WIDTH - 1 - rx_eye
            alpha = white_eye_mask[ry_eye, sourceX]
            if alpha > 0:
                targetColor = COLOR_WHITE.copy()
                pX = rx_eye - 11
                pY = ry_eye - 10
                pSourceX = IRIS_WIDTH - 1 - pX
                if 0 <= pX < IRIS_WIDTH and 0 <= pY < IRIS_HEIGHT:
                    pAlpha = iris_mask[pY, pSourceX]
                    if pAlpha > 0:
                        a2 = pAlpha / 255.0
                        targetColor = COLOR_WHITE * (1.0-a2) + COLOR_PUPIL * a2
                
                a = alpha / 255.0
                color = color * (1.0-a) + targetColor * a
                
        texture[ty, tx] = color

texture = texture.astype(np.uint8)

fps = 60
duration = 4
num_frames = fps * duration

class Spring:
    def __init__(self, p=0, target=0):
        self.pos = p
        self.vel = 0.0
        self.target = target
        self.k = 180.0
        self.d = 12.0
    def update(self, dt):
        force = -self.k * (self.pos - self.target) - self.d * self.vel
        self.vel += force * dt
        self.pos += self.vel * dt

squash = Spring(1.0, 1.0)
blinkLid = Spring(0.0, 0.0)
yaw = 0.0
pitch = 0.0
targetYaw = 0.0
targetPitch = 0.0
prevYaw = 0.0

nextActionTime = 0.0
currentEmotion = 0
smearX = 1.0

def update_physics(t_now):
    global yaw, pitch, targetYaw, targetPitch, prevYaw, smearX, nextActionTime, currentEmotion
    dt = 0.016666
    
    step = dt
    squash.update(step)
    blinkLid.update(step)
    yaw += (targetYaw - yaw) * 6.0 * step
    pitch += (targetPitch - pitch) * 6.0 * step
    
    yawVel = abs(yaw - prevYaw) / step
    smearX = 1.0 + (yawVel * 0.2)
    prevYaw = yaw
    
    if t_now > nextActionTime:
        action = np.random.randint(0, 100)
        if currentEmotion == 1:
            squash.target = 1.0
            targetYaw = np.random.uniform(-1, 1) * 0.8
            targetPitch = np.random.uniform(-1, 1) * 0.4
            squash.vel += 15.0
            currentEmotion = 0
            nextActionTime = t_now + np.random.uniform(0.5, 2.0)
        elif blinkLid.target > 0.5:
            blinkLid.target = 0
            currentEmotion = 0
            nextActionTime = t_now + np.random.uniform(1.0, 3.0)
        else:
            if action < 15:
                squash.target = 0.7
                currentEmotion = 1
                nextActionTime = t_now + 0.15
            elif action < 40:
                blinkLid.target = 1.0
                currentEmotion = 0
                nextActionTime = t_now + 0.12
            else:
                targetYaw = np.random.uniform(-1, 1) * 0.8
                targetPitch = np.random.uniform(-1, 1) * 0.4
                currentEmotion = 0
                nextActionTime = t_now + np.random.uniform(0.5, 2.0)

def render_frame_fast():
    frame = np.zeros((240, 240, 3), dtype=np.uint8)
    
    sinY, cosY = math.sin(yaw), math.cos(yaw)
    sinP, cosP = math.sin(pitch), math.cos(pitch)
    blinkY = int(blinkLid.pos * 240.0)
    sY = max(0.1, squash.pos)
    sX = max(0.1, squash.pos / smearX)
    
    Y, X = np.ogrid[:240, :240]
    
    v = (Y - 120) / 120.0
    u = (X - 120) / 120.0
    r2 = u*u + v*v
    
    mask = r2 <= 1.0
    z = np.zeros_like(r2)
    z[mask] = np.sqrt(1.0 - r2[mask])
    
    rx = u * cosY - z * sinY
    rz = u * sinY + z * cosY
    ry = v * cosP - rz * sinP
    
    tx = (rx * sX * 120.0 + 120.0).astype(np.int32)
    ty = (ry / sY * 120.0 + 120.0).astype(np.int32)
    
    valid = mask & (tx >= 0) & (tx < 240) & (ty >= 0) & (ty < 240) & (Y >= blinkY)
    
    frame[valid] = texture[ty[valid], tx[valid]]
    
    # Scale up for a clearer MP4
    frame_scaled = cv2.resize(frame, (480, 480), interpolation=cv2.INTER_NEAREST)
    return frame_scaled

print("Generating true 1:1 frames...")
os.makedirs('frames', exist_ok=True)
for i in range(num_frames):
    update_physics(i / fps)
    frame = render_frame_fast()
    cv2.imwrite(f'frames/frame_{i:04d}.png', frame)

print("Frames generated. Encoding MP4 with FFmpeg...")
subprocess.run([
    'ffmpeg', '-y', '-framerate', str(fps), '-i', 'frames/frame_%04d.png',
    '-c:v', 'libx264', '-pix_fmt', 'yuv420p', 'preview.mp4'
])

print("Done! Cleaning up...")
for i in range(num_frames):
    os.remove(f'frames/frame_{i:04d}.png')
os.rmdir('frames')
