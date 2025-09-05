# app.py
from fastapi import FastAPI, Request
from fastapi.responses import JSONResponse
from faster_whisper import WhisperModel
import subprocess
import os
import uvicorn
import socket
import threading

app = FastAPI()

# ===== Cấu hình thư mục =====
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
VIDEO_DIR = os.path.join(BASE_DIR, "temp_video")
os.makedirs(VIDEO_DIR, exist_ok=True)

# Load Whisper
model = WhisperModel("base", device="cpu", compute_type="int8")

# ===== Hàm trích xuất audio từ video =====
def extract_audio(video_path: str, audio_path: str):
    cmd = [
        "ffmpeg",
        "-y",
        "-i", video_path,
        "-vn",
        "-acodec", "pcm_s16le",
        "-ar", "16000",
        "-ac", "1",
        audio_path
    ]
    subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

# ===== Luồng nhận video và trả ngay "ok" =====
@app.post("/api/upload", response_class=JSONResponse)
async def upload_video(request: Request):
    try:
        data = await request.body()
        video_filename = "video_received.mp4"
        video_path = os.path.join(VIDEO_DIR, video_filename)

        # Lưu file video
        with open(video_path, "wb") as f:
            f.write(data)

        # Trả ngay cho ESP32
        return {"status": "success", "text": "ok"}

    except Exception as e:
        return {"status": "error", "message": str(e)}

# ===== Luồng xử lý file video thành text =====
def process_video_to_text(video_path: str):
    try:
        audio_filename = "audio_extracted.wav"
        audio_path = os.path.join(VIDEO_DIR, audio_filename)

        extract_audio(video_path, audio_path)

        segments, info = model.transcribe(audio_path, beam_size=5, language="vi")
        text = " ".join([segment.text for segment in segments])
        print("Transcription:", text)
    except Exception as e:
        print("Error in transcription:", str(e))

# ===== Hàm kiểm tra video mới =====
def video_watcher():
    last_mtime = 0
    video_path = os.path.join(VIDEO_DIR, "video_received.mp4")
    while True:
        if os.path.exists(video_path):
            mtime = os.path.getmtime(video_path)
            if mtime != last_mtime:
                last_mtime = mtime
                # Tạo luồng xử lý text
                threading.Thread(target=process_video_to_text, args=(video_path,)).start()

import time
def start_watcher():
    threading.Thread(target=video_watcher, daemon=True).start()

# ===== Lấy IP LAN =====
def get_local_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
    except Exception:
        ip = "127.0.0.1"
    finally:
        s.close()
    return ip

if __name__ == "__main__":
    start_watcher()
    local_ip = get_local_ip()
    print(f"Starting server on LAN IP: {local_ip}")
    uvicorn.run(app, host=local_ip, port=9000)
