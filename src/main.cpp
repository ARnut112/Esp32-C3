#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C // Address ที่สแกนเจอ

#define SDA_PIN 5
#define SCL_PIN 6

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int count = 0;

void setup() {
  Serial.begin(115200);
  delay(2000);

  // เริ่มต้น I2C บนขา GPIO5 และ GPIO6
  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("OLED Allocation Failed"));
    for (;;);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
}

void loop() {
  display.clearDisplay();

  // 1. วาดแถบ Header ด้านบน
  display.fillRect(0, 0, 128, 14, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(18, 3);
  display.print("ESP32-C3 READY");

  // 2. แสดงสถานะอุปกรณ์
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(4, 22);
  display.print("I2C Address: 0x3C");

  display.setCursor(4, 34);
  display.print("Status: Connected");

  // 3. วาด ProgressBar โหลดวิ่งด้านล่าง
  int progress = (count * 10) % 120;
  display.drawRect(4, 48, 120, 10, SSD1306_WHITE);
  display.fillRect(6, 50, progress, 6, SSD1306_WHITE);

  display.display();

  count++;
  delay(200);
}