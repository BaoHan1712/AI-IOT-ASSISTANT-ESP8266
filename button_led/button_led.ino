#define BUTTON_PIN 15
#define LED_PIN    5
#define LED_ON_TIME 5000   // 3 giây

unsigned long ledTimer = 0;      // thời điểm LED bật
bool ledState = false;           // trạng thái LED
bool lastButtonState = HIGH;     // trạng thái nút trước đó

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP); // nút nhấn nối GND
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);        // LED tắt mặc định
}

// Hàm xử lý nút nhấn và LED
void handleButtonLED() {
  bool buttonState = digitalRead(BUTTON_PIN);

  // Phát hiện nhấn nút từ HIGH -> LOW
  if (buttonState == LOW && lastButtonState == HIGH) {
    ledState = true;                 // bật LED
    ledTimer = millis();             // ghi thời điểm bật LED
    digitalWrite(LED_PIN, HIGH);
  }

  lastButtonState = buttonState;

  // Tắt LED sau 3 giây
  if (ledState && millis() - ledTimer >= LED_ON_TIME) {
    ledState = false;
    digitalWrite(LED_PIN, LOW);
  }
}

void loop() {
  handleButtonLED();   // gọi hàm trong loop
}
