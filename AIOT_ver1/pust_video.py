# client_upload.py
import requests

url = "http://127.0.0.1:9000/api/upload"
file_path = "toibuon.mp3"

with open(file_path, "rb") as f:
    files = {"file": (file_path, f, "application/octet-stream")}
    response = requests.post(url, files=files)

print("Kết quả từ server:", response.json())
