#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* ssid     = "Kai 2.4G";
const char* password = "0631573754"; // เปลี่ยนเป็นรหัสผ่านของคุณ

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

#define SDA_PIN 5
#define SCL_PIN 6

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ฟังก์ชันวาดไอคอนความแรงสัญญาณ Wi-Fi (4 ขีด)
void drawWiFiIcon(int x, int y, int rssi) {
  int bars = 0;
  if (rssi >= -55)      bars = 4; // สัญญาณดีมาก
  else if (rssi >= -65) bars = 3; // สัญญาณดี
  else if (rssi >= -75) bars = 2; // สัญญาณปานกลาง
  else if (rssi >= -85) bars = 1; // สัญญาณอ่อน
  else                  bars = 0; // ไม่มีสัญญาณ

  // วาดขีด 4 ขีดที่มุมขวาบน
  for (int i = 0; i < 4; i++) {
    int barHeight = (i + 1) * 3; // ขีดสูงขึ้นทีละ 3px (3, 6, 9, 12px)
    int barX = x + (i * 4);      // แต่ละขีดเว้นระยะ 4px
    int barY = y + (12 - barHeight);

    if (i < bars) {
      display.fillRect(barX, barY, 2, barHeight, SSD1306_WHITE); // ขีดติด
    } else {
      display.drawRect(barX, barY, 2, barHeight, SSD1306_WHITE); // ขีดโปร่ง
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Wire.begin(SDA_PIN, SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("OLED Allocation Failed"));
    for (;;);
  }

  // หน้าจอระหว่างรอเชื่อมต่อ
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 15);
  display.println("Connecting Wi-Fi...");
  display.setCursor(0, 30);
  display.print("SSID: ");
  display.println(ssid);
  display.display();

  // ตั้งค่า Wi-Fi เพื่อความเสถียรของ ESP32-C3 Super Mini
  WiFi.persistent(false);
  WiFi.disconnect(true, true);
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_11dBm);

  WiFi.begin(ssid, password);

  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 40) {
    delay(500);
    timeout++;
  }
}

void loop() {
  display.clearDisplay();

  // 1. แถบ Header: ชื่อ SSID และไอคอน Wi-Fi ที่มุมขวาบน
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 2);
  display.print(ssid);

  if (WiFi.status() == WL_CONNECTED) {
    int rssi = WiFi.RSSI();

    // วาดไอคอน Wi-Fi ที่พิกัด X=110, Y=0
    drawWiFiIcon(110, 0, rssi);

    // เส้นขีดแบ่งโซน
    display.drawFastHLine(0, 16, 128, SSD1306_WHITE);

    // 2. แสดง IP Address
    display.setCursor(0, 24);
    display.print("IP Address:");
    display.setCursor(0, 36);
    display.print(WiFi.localIP().toString());

    // 3. แสดงค่า RSSI เป็น dBm
    display.setCursor(0, 52);
    display.printf("Signal: %d dBm", rssi);

  } else {
    // หากสัญญาณหลุด
    display.drawFastHLine(0, 16, 128, SSD1306_WHITE);
    display.setCursor(0, 32);
    display.print("Status: Disconnected");
  }

  display.display();
  delay(2000); // อัปเดตสถานะและระดับสัญญาณทุกๆ 2 วินาที
}