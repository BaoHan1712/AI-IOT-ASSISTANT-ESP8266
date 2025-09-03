  #include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// ====== Cấu hình WiFi và Gemini API ======
const String ssid = "Viet Bao";              // Tên mạng WiFi
const String password = "thongnhat75";      // Mật khẩu WiFi
const String API_key = "AIzaSyDqUhv2o9Ybk1h0dcupMsYQ3fFbc6rIFKQ";      // API key lấy từ Google AI Studio

// ====== Hàm kết nối WiFi ======
void setupWifi() {
  WiFi.begin(ssid, password);                 // Kết nối tới WiFi
  while (WiFi.status() != WL_CONNECTED) {     // Chờ cho đến khi kết nối thành công
    delay(1000);
    Serial.print("...");
  }
  Serial.print("IP address: ");               // In địa chỉ IP sau khi kết nối
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);                        // ESP8266 hoạt động ở chế độ station (client)
  WiFi.disconnect();                          // Ngắt kết nối cũ (nếu có)

  while (!Serial) ;                           // Chờ Serial sẵn sàng (để nhập prompt)

  setupWifi();                                // Gọi hàm kết nối WiFi
}

// ====== Hàm gửi request tới Gemini API ======
void makeGeminiRequest(String input) {
  WiFiClientSecure client;                    // Client bảo mật (HTTPS)
  client.setInsecure();                       // Bỏ qua SSL verify (demo)
  HTTPClient http;                            // HTTP client

  // Bắt đầu kết nối đến endpoint của Gemini
  if (http.begin(client, "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key=" + API_key)) {
    http.addHeader("Content-Type", "application/json");   // Header JSON
    // JSON body với prompt do người dùng nhập
    String payload = String("{\"contents\": [{\"parts\":[{\"text\":\"" + input + "\"}]}]}");

    int httpCode = http.POST(payload);        // Gửi POST request

    // Nếu thành công (HTTP 200)
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();      // Lấy response dạng JSON (chuỗi)

      Serial.println("Payload: " + payload);  // In toàn bộ response (debug)

      // Parse JSON để lấy nội dung text trả về
      DynamicJsonDocument doc(2048);          
      deserializeJson(doc, payload);
      String responseText = doc["candidates"][0]["content"]["parts"][0]["text"];

      // In câu trả lời của Gemini
      Serial.print("Response: ");
      Serial.println(responseText);
    } else {
      Serial.println("Failed to reach Gemini"); // Trường hợp lỗi
    }

    http.end();                               // Đóng kết nối
  } else {
    Serial.print("Unable to connect\n");       // Không kết nối được server
  }
}

void loop() {
    if (Serial.available() > 0) {
      // Đọc prompt từ Serial
      String sentence = Serial.readStringUntil('\n'); 

      Serial.print("You entered: ");
      Serial.println(sentence);

      // Gửi prompt đến Gemini API
      makeGeminiRequest(sentence);
      Serial.println("Enter another sentence:"); 
  }
}
