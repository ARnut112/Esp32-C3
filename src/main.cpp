#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C // I2C Address ปกติของ OLED 0.96" (บางรุ่นเป็น 0x3D)

// กำหนดขา I2C สำหรับ ESP32-C3 Super Mini
#define SDA_PIN 5
#define SCL_PIN 6

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int counter = 0;

void setup() {
  Serial.begin(115200);
  delay(2000);

  // กำหนดขา I2C ก่อนเรียกใช้งานจอ
  Wire.begin(SDA_PIN, SCL_PIN);

  // เริ่มต้นใช้งานจอ OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("[ERROR] OLED SSD1306 allocation failed!"));
    for (;;); // วนลูปค้างไว้หากหาจอไม่พบ
  }

  Serial.println(F("[SUCCESS] OLED Initialized!"));

  // เคลียร์หน้าจอ และแสดงโลโก้เริ่มต้น
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // วาดเส้นกรอบหน้าจอ
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  
  // พิมพ์ข้อความต้อนรับ
  display.setCursor(10, 12);
  display.println("ESP32-C3 SuperMini");
  display.setCursor(20, 28);
  display.println("OLED 0.96\" Test");
  display.display();
  delay(2000);
}

void loop() {
  display.clearDisplay();
  
  // วาดกรอบด้านนอก
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  
  // หัวข้อ
  display.setTextSize(1);
  display.setCursor(15, 10);
  display.println("--- STATUS OK ---");
  
  // แสดงตัวนับเวลา (Counter)
  display.setTextSize(2);
  display.setCursor(25, 30);
  display.printf("SEC: %d", counter);
  
  display.display();
  
  counter++;
  delay(1000);
}