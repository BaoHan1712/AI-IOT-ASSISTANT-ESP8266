#include <driver/i2s.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>   

// ===== Prototype =====
void wavHeader(byte* header, int wavSize);
void startRecording();
void listSPIFFS(void);
void uploadFile(const char* path);
void i2sInit();
void i2s_adc(void *arg);
void i2s_adc_data_scale(uint8_t * d_buff, uint8_t* s_buff, uint32_t len);

// ===== Pin & cấu hình =====
#define I2S_WS   32
#define I2S_SD   34
#define I2S_SCK  33  
#define BUTTON_PIN 15
#define LED_PIN    5
#define LED_PIN_2   2
#define LED_PIN_3    4


#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE   (16000)
#define I2S_SAMPLE_BITS   (16)
#define I2S_READ_LEN      (2 * 1024)
#define RECORD_TIME       (6) //Seconds
#define I2S_CHANNEL_NUM   (1)
#define FLASH_RECORD_SIZE (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * RECORD_TIME)

// TCP server để lắng nghe gói tin điều khiển
WiFiServer controlServer(8080);
TaskHandle_t controlTaskHandle = NULL;

File file;
const char filename[] = "/recording.wav";
const int headerSize = 44;

// WiFi
const char* ssid     = "Viet Bao";
const char* password = "thongnhat75";

// Server
const char* serverUrl = "http://192.168.1.2:9000/api/upload";

// ===== State =====
bool lastButtonState = HIGH;
TaskHandle_t i2sTaskHandle = NULL;

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(LED_PIN_2, OUTPUT);
  digitalWrite(LED_PIN_2, LOW);
  pinMode(LED_PIN_3, OUTPUT);
  digitalWrite(LED_PIN_3, LOW);

  // Kết nối WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);  // quan trọng: tránh reset watchdog
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());

  // Sau khi WiFi OK mới khởi động server TCP
  controlServer.begin();
  Serial.println("Control server started on port 8080");

  // Tạo task để lắng nghe dữ liệu
  xTaskCreatePinnedToCore(controlTask, "controlTask", 4096, NULL, 0, &controlTaskHandle, 0);

  // SPIFFS và I2S
  if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS init failed!");
    while(1) yield();
  }
  i2sInit();
  listSPIFFS();
}


void loop() {
  bool buttonState = digitalRead(BUTTON_PIN);

  // Nhấn nút từ HIGH -> LOW
  if (buttonState == LOW && lastButtonState == HIGH) {
    Serial.println("Button pressed, start recording...");
    digitalWrite(LED_PIN, HIGH); // LED sáng khi ghi âm

    // Mở file mới với header WAV
    startRecording();

    // Tạo task thu âm nếu chưa có
    if (i2sTaskHandle == NULL) {
      xTaskCreate(i2s_adc, "i2s_adc", 1024*4, NULL, 1, &i2sTaskHandle);
    }
  }

  lastButtonState = buttonState;
  delay(10); // debounce đơn giản
}

void controlTask(void *pvParameters) {
  while (true) {
    WiFiClient client = controlServer.available();
    if (client) {
      Serial.println("Control client connected");
      while (client.connected()) {
        if (client.available() >= 3) {
          uint8_t startByte = client.read();
          uint8_t dataByte  = client.read();
          uint8_t endByte   = client.read();

          Serial.printf("Recv: 0x%02X 0x%02X 0x%02X\n", startByte, dataByte, endByte);

          if (startByte == 0x02 && endByte == 0x03) {
            if (dataByte == '1') {
              digitalWrite(LED_PIN, HIGH);
              Serial.println("LED RED ON");
            } else if (dataByte == '0') {
              digitalWrite(LED_PIN, LOW);
              Serial.println("LED RED OFF");
            }
            else if (dataByte == '3') {
              digitalWrite(LED_PIN_2, HIGH);
              Serial.println("LED yellow ON");
            }
            else if (dataByte == '4') {
              digitalWrite(LED_PIN_2, LOW);
              Serial.println("LED yellow OFF");
            }
            else if (dataByte == '5') {
              digitalWrite(LED_PIN_3, HIGH);
              Serial.println("LED blue ON");
            }
            else if (dataByte == '6') {
              digitalWrite(LED_PIN_3, LOW);
              Serial.println("LED blue OFF");
            }
          }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
      }
      client.stop();
      Serial.println("Control client disconnected");
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}


// ===== Mở file WAV =====
void startRecording() {
  SPIFFS.remove(filename);
  file = SPIFFS.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Cannot open file for recording!");
    return;
  }
  byte header[headerSize];
  wavHeader(header, FLASH_RECORD_SIZE);
  file.write(header, headerSize);
}

// ===== I2S =====
void i2sInit(){
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = i2s_bits_per_sample_t(I2S_SAMPLE_BITS),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = 0,
    .dma_buf_count = 64,
    .dma_buf_len = 1024,
    .use_apll = 1
  };
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };
  i2s_set_pin(I2S_PORT, &pin_config);
}

// ===== Chuyển dữ liệu ADC =====
void i2s_adc_data_scale(uint8_t * d_buff, uint8_t* s_buff, uint32_t len) {
  uint32_t j = 0;
  uint32_t dac_value = 0;
  for (int i = 0; i < len; i += 2) {
    dac_value = ((((uint16_t) (s_buff[i + 1] & 0xf) << 8) | ((s_buff[i + 0]))));
    d_buff[j++] = 0;
    d_buff[j++] = dac_value * 256 / 2048;
  }
}

// ===== Task thu âm =====
void i2s_adc(void *arg) {
  int i2s_read_len = I2S_READ_LEN;
  int flash_wr_size = 0;
  size_t bytes_read;

  char* i2s_read_buff = (char*) calloc(i2s_read_len, sizeof(char));
  uint8_t* flash_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));
  if(!i2s_read_buff || !flash_write_buff){
    Serial.println("Memory allocation failed!");
    vTaskDelete(NULL);
  }

  // warm-up discard
  i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
  i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);

  Serial.println(" *** Recording Start *** ");
  while (flash_wr_size < FLASH_RECORD_SIZE) {
    i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
    i2s_adc_data_scale(flash_write_buff, (uint8_t*)i2s_read_buff, i2s_read_len);
    file.write((const byte*) flash_write_buff, i2s_read_len);
    flash_wr_size += i2s_read_len;
    ets_printf("Recording %u%%\n", flash_wr_size * 100 / FLASH_RECORD_SIZE);
  }

  file.close();
  free(i2s_read_buff);
  free(flash_write_buff);

  digitalWrite(LED_PIN, LOW); // tắt LED
  i2sTaskHandle = NULL;       // reset handle

  listSPIFFS();
  uploadFile(filename);

  vTaskDelete(NULL);
}

// ===== Upload =====
void uploadFile(const char* path) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, cannot upload!");
    return;
  }

  File f = SPIFFS.open(path, FILE_READ);
  if (!f) {
    Serial.println("Failed to open file for upload!");
    return;
  }

  HTTPClient http;
  http.begin(serverUrl);
  http.setTimeout(30000);
  http.addHeader("Content-Type", "audio/wav");

  Serial.println("Uploading file...");
  int httpResponseCode = http.sendRequest("POST", &f, f.size());

  if (httpResponseCode > 0) {
    Serial.printf("✅ Upload done, code: %d\n", httpResponseCode);
    String response = http.getString();
    Serial.println(response);

    // Parse JSON
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, response);
    if (!error) {
      const char* text = doc["text"];
      if (text) Serial.println("Result: " + String(text));
    }
  } else {
    Serial.printf("❌ Upload failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
  }

  f.close();
  http.end();
}

// ===== WAV header =====
void wavHeader(byte* header, int wavSize){
  header[0] = 'R'; header[1] = 'I'; header[2] = 'F'; header[3] = 'F';
  unsigned int fileSize = wavSize + headerSize - 8;
  header[4] = (byte)(fileSize & 0xFF); header[5] = (byte)((fileSize >> 8) & 0xFF);
  header[6] = (byte)((fileSize >> 16) & 0xFF); header[7] = (byte)((fileSize >> 24) & 0xFF);
  header[8] = 'W'; header[9] = 'A'; header[10] = 'V'; header[11] = 'E';
  header[12] = 'f'; header[13] = 'm'; header[14] = 't'; header[15] = ' ';
  header[16] = 0x10; header[17] = 0x00; header[18] = 0x00; header[19] = 0x00;
  header[20] = 0x01; header[21] = 0x00; header[22] = 0x01; header[23] = 0x00;
  header[24] = 0x80; header[25] = 0x3E; header[26] = 0x00; header[27] = 0x00;
  header[28] = 0x00; header[29] = 0x7D; header[30] = 0x00; header[31] = 0x00;
  header[32] = 0x02; header[33] = 0x00; header[34] = 0x10; header[35] = 0x00;
  header[36] = 'd'; header[37] = 'a'; header[38] = 't'; header[39] = 'a';
  header[40] = (byte)(wavSize & 0xFF); header[41] = (byte)((wavSize >> 8) & 0xFF);
  header[42] = (byte)((wavSize >> 16) & 0xFF); header[43] = (byte)((wavSize >> 24) & 0xFF);
}

// ===== List SPIFFS =====
void listSPIFFS(void) {
  Serial.println(F("\nListing SPIFFS files:"));
  static const char line[] PROGMEM = "=================================================";
  Serial.println(FPSTR(line));
  fs::File root = SPIFFS.open("/");
  if (!root || !root.isDirectory()) return;

  fs::File f = root.openNextFile();
  while(f){
    Serial.print(f.isDirectory() ? "DIR : " : "FILE: ");
    Serial.print(f.name());
    Serial.print("  ");
    Serial.println(f.size());
    f = root.openNextFile();
  }
  Serial.println(FPSTR(line));
}