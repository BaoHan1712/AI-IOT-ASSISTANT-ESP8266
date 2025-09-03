# app.py
from fastapi import FastAPI, File, UploadFile, Request
from fastapi.responses import HTMLResponse, JSONResponse
from fastapi.staticfiles import StaticFiles
from fastapi.templating import Jinja2Templates
import shutil
import os
import uvicorn
from faster_whisper import WhisperModel  # <-- mô hình ASR

app = FastAPI()

# Cấu hình thư mục
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
UPLOAD_DIR = os.path.join(BASE_DIR, "uploads")
TEMPLATES_DIR = os.path.join(BASE_DIR, "templates")
STATIC_DIR = os.path.join(BASE_DIR, "static")

os.makedirs(UPLOAD_DIR, exist_ok=True)
os.makedirs(STATIC_DIR, exist_ok=True)

# Mount thư mục static để load CSS/JS/img
app.mount("/static", StaticFiles(directory=STATIC_DIR), name="static")

# Load template Jinja2
templates = Jinja2Templates(directory=TEMPLATES_DIR)

# Load mô hình Whisper (base cho tiếng Việt, có thể đổi sang "small", "medium")
model = WhisperModel("base", device="cpu", compute_type="int8")


# Trang chính (render HTML)
@app.get("/", response_class=HTMLResponse)
async def main(request: Request, msg: str = None):
    return templates.TemplateResponse("index.html", {"request": request, "msg": msg})


# API Upload cho frontend (trả JSON text nhận diện)
@app.post("/api/upload", response_class=JSONResponse)
async def upload_api(file: UploadFile = File(...)):
    try:
        # Lưu file
        save_path = os.path.join(UPLOAD_DIR, file.filename)
        with open(save_path, "wb") as buffer:
            shutil.copyfileobj(file.file, buffer)

        # Nhận diện giọng nói
        segments, info = model.transcribe(save_path, beam_size=5, language="vi")
        text = " ".join([segment.text for segment in segments])

        return {"status": "success", "filename": file.filename, "text": text}
    except Exception as e:
        return {"status": "error", "message": str(e)}


if __name__ == "__main__":
    uvicorn.run(app, host="127.0.0.1", port=9000)
