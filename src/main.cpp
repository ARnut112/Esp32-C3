#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* ssid     = "ar_nut";
const char* password = "YOUR_WIFI_PASSWORD";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

#define SDA_PIN 5
#define SCL_PIN 6
#define BUTTON_PIN 4 // ขาต่อปุ่มกด Tactile Switch

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ตัวแปรจัดการหน้าจอ และระบบ Debounce ปุ่มกด
int currentPage = 0;
const int TOTAL_PAGES = 3;

bool lastButtonState = HIGH;
bool currentButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50; // หน่วงเวลาเช็กปุ่มกด 50ms

// --- ฟังก์ชันวาดหน้าจอแต่ละหน้า ---
void drawPage0() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("PAGE 1: SYSTEM INFO");
  display.drawFastHLine(0, 12, 128, SSD1306_WHITE);

  display.setCursor(0, 22);
  display.println("Device: ESP32-C3");
  display.setCursor(0, 36);
  display.println("Status: Active");
  display.setCursor(0, 50);
  display.println("[Press button -> Next]");
}

void drawPage1() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("PAGE 2: WI-FI STATUS");
  display.drawFastHLine(0, 12, 128, SSD1306_WHITE);

  if (WiFi.status() == WL_CONNECTED) {
    display.setCursor(0, 20);
    display.print("SSID: "); display.println(ssid);
    display.setCursor(0, 32);
    display.print("IP: "); display.println(WiFi.localIP());
    display.setCursor(0, 44);
    display.printf("RSSI: %d dBm", WiFi.RSSI());
  } else {
    display.setCursor(0, 30);
    display.println("Wi-Fi: Disconnected");
  }
}

void drawPage2() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("PAGE 3: CONTROL MENU");
  display.drawFastHLine(0, 12, 128, SSD1306_WHITE);

  display.setCursor(0, 22);
  display.println("> [1] Relay 1: OFF");
  display.setCursor(0, 36);
  display.println("  [2] Relay 2: OFF");
  display.setCursor(0, 50);
  display.println("  [3] Home Assistant");
}

void renderCurrentPage() {
  display.clearDisplay();
  switch (currentPage) {
    case 0: drawPage0(); break;
    case 1: drawPage1(); break;
    case 2: drawPage2(); break;
  }
  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  // ตั้งค่าขาปุ่มกดแบบ INPUT_PULLUP (กด = LOW, ไม่กด = HIGH)
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Wire.begin(SDA_PIN, SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("OLED Allocation Failed"));
    for (;;);
  }

  // ตั้งค่า Wi-Fi เพื่อความเสถียร
  WiFi.persistent(false);
  WiFi.disconnect(true, true);
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_11dBm);
  WiFi.begin(ssid, password);

  renderCurrentPage();
}

void loop() {
  // อ่านค่าปุ่มกดและตัดสัญญาณรบกวน (Debounce)
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != currentButtonState) {
      currentButtonState = reading;

      // ตรวจพบจังหวะ "กดปุ่ม" (Transition จาก HIGH ไป LOW)
      if (currentButtonState == LOW) {
        currentPage = (currentPage + 1) % TOTAL_PAGES; // สลับหน้า 0 -> 1 -> 2 -> 0
        renderCurrentPage();
        Serial.printf("Switched to Page: %d\n", currentPage + 1);
      }
    }
  }

  lastButtonState = reading;
}