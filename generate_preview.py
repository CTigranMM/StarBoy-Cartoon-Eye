import numpy as np
import cv2
import math
import subprocess
import os

width = 240
height = 240
fps = 60
duration = 4 # 4 seconds
num_frames = fps * duration

# Colors
bg_color = (0, 0, 0)
white_color = (253, 249, 249) # BGR in cv2 is (249, 249, 253) but let's use RGB for ffmpeg raw or cv2
pupil_color = (67, 120, 98)   # RGB

os.makedirs('frames', exist_ok=True)

def get_frame(t):
    frame = np.zeros((height, width, 3), dtype=np.uint8)
    
    # Simple animation logic
    yaw = math.sin(t * math.pi) * 0.5
    pitch = math.cos(t * math.pi * 0.7) * 0.3
    
    blink = 0.0
    if (t % 2) > 1.8:
        blink = 1.0 - abs((t % 2) - 1.9)*10.0
        blink = max(0.0, min(1.0, blink))
        
    squash = 1.0 - math.sin(t * math.pi * 2) * 0.08
    smearX = 1.0 + abs(math.cos(t * math.pi)) * 0.1
    
    X, Y = np.meshgrid(np.arange(width), np.arange(height))
    
    u = (X - 120.0) / 120.0
    v = (Y - 120.0) / 120.0
    r2 = u**2 + v**2
    
    mask = r2 <= 1.0
    z = np.zeros_like(r2)
    z[mask] = np.sqrt(1.0 - r2[mask])
    
    sinY, cosY = math.sin(yaw), math.cos(yaw)
    sinP, cosP = math.sin(pitch), math.cos(pitch)
    
    rx = u * cosY - z * sinY
    rz = u * sinY + z * cosY
    ry = v * cosP - rz * sinP
    
    sY = squash
    sX = squash / smearX
    
    tx = (rx * sX * 120.0 + 120.0).astype(np.int32)
    ty = (ry / sY * 120.0 + 120.0).astype(np.int32)
    
    valid = mask & (tx >= 0) & (tx < 240) & (ty >= 0) & (ty < 240)
    valid &= (Y > blink * 240)
    
    lx = tx - 67
    ly = ty - 121
    rx_eye = tx - 173
    ry_eye = ty - 121
    
    # Eye masks (slightly taller than wide)
    left_eye_mask = (lx**2 / (38**2) + ly**2 / (43**2)) <= 1.0
    right_eye_mask = (rx_eye**2 / (38**2) + ry_eye**2 / (43**2)) <= 1.0
    
    # Pupil masks (inner facing)
    lp_x = tx - 74
    lp_y = ty - 121
    rp_x = tx - 166
    rp_y = ty - 121
    left_pupil_mask = (lp_x**2 / (15**2) + lp_y**2 / (35**2)) <= 1.0
    right_pupil_mask = (rp_x**2 / (15**2) + rp_y**2 / (35**2)) <= 1.0
    
    eye_pixels = valid & (left_eye_mask | right_eye_mask)
    pupil_pixels = valid & (left_pupil_mask | right_pupil_mask)
    
    # Use BGR for cv2.imwrite
    frame[eye_pixels] = (249, 249, 253)
    frame[pupil_pixels] = (67, 120, 98)[::-1] 
    
    # Antialiasing blur to make it look nicer like the real screen
    frame = cv2.GaussianBlur(frame, (3, 3), 0)
    
    return frame

print("Generating frames...")
for i in range(num_frames):
    t = i / fps
    frame = get_frame(t)
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
