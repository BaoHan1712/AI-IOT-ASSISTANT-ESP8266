#include "AudioTools.h"
#include "BluetoothA2DPSink.h"

// Chân I2S kết nối MAX98357A
#define I2S_BCLK  27
#define I2S_LRC   26
#define I2S_DOUT  25

I2SStream i2s;                 // luồng I2S output
BluetoothA2DPSink a2dp_sink(i2s); // gửi âm thanh BT -> I2S

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Bluetooth Speaker starting...");

  // Cấu hình I2S output
  auto cfg = i2s.defaultConfig(TX_MODE);
  cfg.pin_bck  = I2S_BCLK;
  cfg.pin_ws   = I2S_LRC;
  cfg.pin_data = I2S_DOUT;
  cfg.sample_rate = 44100;
  cfg.bits_per_sample = 16;
  cfg.channels = 2;

  i2s.begin(cfg);

  // Khởi động loa Bluetooth
  a2dp_sink.set_auto_reconnect(true);
  a2dp_sink.set_volume(90);   // 0–127
  a2dp_sink.start("ESP32_BT_Loudspeaker");

  Serial.println("ESP32 đã sẵn sàng làm loa Bluetooth!");
}

void loop() {
  delay(1000);
}
