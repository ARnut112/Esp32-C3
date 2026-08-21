#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_system.h>
#include <ESP32Ping.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ============================================================
// HARDWARE
// ============================================================

#define BUTTON_A 4
#define BUTTON_B 3

#define OLED_SDA 20
#define OLED_SCL 10

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    -1
);

// ============================================================
// WIFI SETTINGS
// ============================================================

const unsigned long WIFI_TIMEOUT_MS = 15000;

#define MAX_WIFI_NETWORKS 20

int wifiCount = 0;
int wifiIndex = 0;

String wifiNames[MAX_WIFI_NETWORKS];
int wifiRSSI[MAX_WIFI_NETWORKS];

String wifiPassword = "";

// ============================================================
// PASSWORD KEYBOARD
// ============================================================

const char* passwordSets[] =
{
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
    "abcdefghijklmnopqrstuvwxyz",
    "0123456789",
    "!@#$%^&*_-."
};

const int PASSWORD_SET_COUNT = 4;

int passwordSet = 0;
int passwordIndex = 0;

// ============================================================
// MAIN MENU
// ============================================================

const char* mainMenu[] =
{
    "Wi-Fi",
    "Network",
    "Password",
    "Stopwatch"
};

const int MAIN_MENU_COUNT = 4;

int menuIndex = 0;

// ============================================================
// NETWORK MENU
// ============================================================

const char* networkMenu[] =
{
    "Network Info",
    "LAN Devices",
    "Ping Device"
};

const int NETWORK_MENU_COUNT = 3;

int networkMenuIndex = 0;

// ============================================================
// LAN DEVICES
// ============================================================

#define MAX_LAN_DEVICES 32

IPAddress lanDevices[MAX_LAN_DEVICES];

int lanDeviceCount = 0;
int lanDeviceIndex = 0;

bool lanScanRunning = false;

// ============================================================
// PASSWORD GENERATOR
// ============================================================

const char passwordChars[] =
    "ABCDEFGHJKLMNPQRSTUVWXYZ"
    "abcdefghijkmnopqrstuvwxyz"
    "23456789"
    "!@#$%&*_-";

String generatedPassword = "";

const int passwordLengths[] =
{
    8,
    12,
    16
};

int passwordLengthIndex = 1;

// ============================================================
// STOPWATCH
// ============================================================

bool stopwatchRunning = false;

unsigned long stopwatchStart = 0;
unsigned long stopwatchElapsed = 0;

// ============================================================
// SCREEN STATE
// ============================================================

enum Screen
{
    MAIN_MENU,

    WIFI_MENU,
    WIFI_SCAN,
    WIFI_SELECTED,
    WIFI_PASSWORD,
    WIFI_CONNECTING,
    WIFI_RESULT,

    NETWORK_MENU,
    NETWORK_INFO,
    LAN_SCAN,
    LAN_LIST,
    LAN_DEVICE_INFO,
    PING_RESULT,

    PASSWORD_MENU,
    PASSWORD_GENERATOR,

    STOPWATCH_MENU
};

Screen currentScreen = MAIN_MENU;

// ============================================================
// BUTTON SYSTEM
// ============================================================

struct Button
{
    uint8_t pin;
    bool lastState;
    bool longPressHandled;
    unsigned long pressStart;
};

Button buttonA =
{
    BUTTON_A,
    HIGH,
    false,
    0
};

Button buttonB =
{
    BUTTON_B,
    HIGH,
    false,
    0
};

const unsigned long LONG_PRESS_TIME = 1000;

// ============================================================
// FUNCTION PROTOTYPES
// ============================================================

void drawScreen();

void drawMainMenu();

void drawWiFiMenu();
void drawWiFiList();
void drawSelectedWiFi();
void drawPasswordKeyboard();
void drawConnectionResult();

void drawNetworkMenu();
void drawNetworkInfo();
void drawLANScan();
void drawLANList();
void drawLANDeviceInfo();
void drawPingResult();

void drawPasswordMenu();
void drawPasswordGenerator();

void drawStopwatchMenu();

void startWiFiScan();
void startWiFiConnection();

void startLANScan();
void pingSelectedDevice();

void generatePassword();

void updateStopwatch();

void enterMainMenu();
void enterNetworkMenu();
void enterPasswordMenu();

void goBack();

bool buttonPressed(Button &button);
bool buttonLongPressed(Button &button);

void drawHeader(const char* title);

// ============================================================
// BUTTON SHORT PRESS
// ============================================================

bool buttonPressed(Button &button)
{
    bool state = digitalRead(button.pin);

    bool result = false;

    if (
        button.lastState == HIGH &&
        state == LOW
    )
    {
        button.pressStart = millis();
        button.longPressHandled = false;
    }

    if (
        button.lastState == LOW &&
        state == HIGH
    )
    {
        unsigned long duration =
            millis() - button.pressStart;

        if (duration < LONG_PRESS_TIME)
        {
            result = true;
        }
    }

    button.lastState = state;

    return result;
}

// ============================================================
// BUTTON LONG PRESS
// ============================================================

bool buttonLongPressed(Button &button)
{
    bool state = digitalRead(button.pin);

    if (
        state == LOW &&
        !button.longPressHandled &&
        millis() - button.pressStart >= LONG_PRESS_TIME
    )
    {
        button.longPressHandled = true;

        return true;
    }

    return false;
}

// ============================================================
// HEADER
// ============================================================

void drawHeader(const char* title)
{
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(4, 0);
    display.println(title);

    display.drawLine(
        0,
        10,
        127,
        10,
        SSD1306_WHITE
    );
}

// ============================================================
// MAIN MENU
// ============================================================

void drawMainMenu()
{
    display.clearDisplay();

    drawHeader("POCKET CYBER DECK");

    for (
        int i = 0;
        i < MAIN_MENU_COUNT;
        i++
    )
    {
        int y = 17 + (i * 11);

        display.setCursor(6, y);

        if (i == menuIndex)
        {
            display.print("> ");
        }
        else
        {
            display.print("  ");
        }

        display.println(mainMenu[i]);
    }

    display.display();
}

// ============================================================
// WIFI MENU
// ============================================================

void drawWiFiMenu()
{
    display.clearDisplay();

    drawHeader("WI-FI");

    display.setCursor(6, 20);
    display.println("> Scan Networks");

    display.setCursor(6, 32);
    display.println("  Saved Network");

    display.setCursor(6, 52);
    display.println("A: Back");

    display.display();
}

// ============================================================
// WIFI SCAN
// ============================================================

void startWiFiScan()
{
    currentScreen = WIFI_SCAN;

    display.clearDisplay();

    drawHeader("WI-FI");

    display.setCursor(15, 28);
    display.println("Scanning...");

    display.display();

    Serial.println();
    Serial.println("==========================");
    Serial.println("Starting Wi-Fi scan...");
    Serial.println("==========================");

    WiFi.persistent(false);

    WiFi.disconnect(
        true,
        true
    );

    delay(100);

    WiFi.mode(WIFI_STA);

    WiFi.setTxPower(
        WIFI_POWER_11dBm
    );

    WiFi.setSleep(false);

    int foundNetworks =
        WiFi.scanNetworks();

    wifiCount = foundNetworks;

    if (
        wifiCount >
        MAX_WIFI_NETWORKS
    )
    {
        wifiCount =
            MAX_WIFI_NETWORKS;
    }

    Serial.print("Networks found: ");
    Serial.println(foundNetworks);

    for (
        int i = 0;
        i < wifiCount;
        i++
    )
    {
        wifiNames[i] =
            WiFi.SSID(i);

        wifiRSSI[i] =
            WiFi.RSSI(i);

        Serial.print(i);
        Serial.print(": ");

        Serial.print(
            wifiNames[i]
        );

        Serial.print(" | ");

        Serial.print(
            wifiRSSI[i]
        );

        Serial.println(" dBm");
    }

    WiFi.scanDelete();

    wifiIndex = 0;

    drawWiFiList();
}

// ============================================================
// WIFI LIST
// ============================================================

void drawWiFiList()
{
    display.clearDisplay();

    drawHeader("WIFI NETWORKS");

    if (wifiCount <= 0)
    {
        display.setCursor(10, 28);
        display.println(
            "No networks found"
        );

        display.setCursor(10, 52);
        display.println("A: Back");

        display.display();

        return;
    }

    int startIndex = 0;

    if (wifiIndex >= 3)
    {
        startIndex =
            wifiIndex - 2;
    }

    for (
        int row = 0;
        row < 4;
        row++
    )
    {
        int i =
            startIndex + row;

        if (i >= wifiCount)
        {
            break;
        }

        int y =
            17 + (row * 11);

        display.setCursor(
            1,
            y
        );

        if (i == wifiIndex)
        {
            display.print("> ");
        }
        else
        {
            display.print("  ");
        }

        String name =
            wifiNames[i];

        if (name.length() > 13)
        {
            name =
                name.substring(
                    0,
                    13
                );
        }

        display.print(name);

        display.setCursor(
            102,
            y
        );

        display.print(
            wifiRSSI[i]
        );
    }

    display.display();
}

// ============================================================
// SELECTED WIFI
// ============================================================

void drawSelectedWiFi()
{
    display.clearDisplay();

    drawHeader("SELECTED WIFI");

    String name =
        wifiNames[wifiIndex];

    if (name.length() > 18)
    {
        name =
            name.substring(
                0,
                18
            );
    }

    display.setCursor(3, 20);
    display.println(name);

    display.setCursor(3, 32);

    display.print(
        wifiRSSI[wifiIndex]
    );

    display.println(" dBm");

    display.setCursor(3, 50);

    display.println(
        "B: Enter Password"
    );

    display.display();
}

// ============================================================
// PASSWORD KEYBOARD
// ============================================================

void drawPasswordKeyboard()
{
    display.clearDisplay();

    drawHeader("PASSWORD");

    display.setCursor(2, 14);

    String name =
        wifiNames[wifiIndex];

    if (name.length() > 18)
    {
        name =
            name.substring(
                0,
                18
            );
    }

    display.println(name);

    display.setCursor(2, 25);

    display.print("Pass:");

    // Show password directly
    display.print(
        wifiPassword
    );

    const char* chars =
        passwordSets[passwordSet];

    int length =
        strlen(chars);

    int start =
        passwordIndex - 4;

    if (start < 0)
    {
        start = 0;
    }

    if (
        start >
        length - 9
    )
    {
        start =
            length - 9;

        if (start < 0)
        {
            start = 0;
        }
    }

    for (
        int i = 0;
        i < 9;
        i++
    )
    {
        int index =
            start + i;

        if (
            index >= length
        )
        {
            break;
        }

        int x =
            4 + (i * 13);

        display.setCursor(
            x,
            38
        );

        display.print(
            chars[index]
        );

        if (
            index ==
            passwordIndex
        )
        {
            display.drawLine(
                x,
                48,
                x + 5,
                48,
                SSD1306_WHITE
            );
        }
    }

    display.setCursor(
        2,
        56
    );

    display.print("A:");

    if (passwordSet == 0)
    {
        display.print("ABC");
    }
    else if (passwordSet == 1)
    {
        display.print("abc");
    }
    else if (passwordSet == 2)
    {
        display.print("123");
    }
    else
    {
        display.print("SYM");
    }

    display.print(" B:OK");

    display.display();
}

// ============================================================
// WIFI CONNECTION
// ============================================================

void startWiFiConnection()
{
    currentScreen =
        WIFI_CONNECTING;

    display.clearDisplay();

    drawHeader("CONNECTING");

    String name =
        wifiNames[wifiIndex];

    if (name.length() > 18)
    {
        name =
            name.substring(
                0,
                18
            );
    }

    display.setCursor(
        3,
        25
    );

    display.println(name);

    display.setCursor(
        3,
        40
    );

    display.println(
        "Please wait..."
    );

    display.display();

    Serial.println();
    Serial.println(
        "=========================="
    );

    Serial.println(
        "Connecting to Wi-Fi"
    );

    Serial.print("SSID: ");

    Serial.println(
        wifiNames[wifiIndex]
    );

    Serial.println(
        "=========================="
    );

    WiFi.persistent(false);

    WiFi.disconnect(
        true,
        true
    );

    delay(100);

    WiFi.mode(WIFI_STA);

    WiFi.setTxPower(
        WIFI_POWER_11dBm
    );

    WiFi.setSleep(false);

    WiFi.begin(
        wifiNames[wifiIndex].c_str(),
        wifiPassword.c_str()
    );

    unsigned long startTime =
        millis();

    while (
        WiFi.status() !=
        WL_CONNECTED
    )
    {
        if (
            millis() -
            startTime >=
            WIFI_TIMEOUT_MS
        )
        {
            Serial.println(
                "\n[TIMEOUT] Wi-Fi Connection failed!"
            );

            break;
        }

        delay(250);

        Serial.print(".");
    }

    Serial.println();

    if (
        WiFi.status() ==
        WL_CONNECTED
    )
    {
        Serial.println(
            "CONNECTED"
        );

        Serial.print("IP: ");

        Serial.println(
            WiFi.localIP()
        );

        Serial.print(
            "RSSI: "
        );

        Serial.println(
            WiFi.RSSI()
        );
    }
    else
    {
        Serial.println(
            "CONNECTION FAILED"
        );
    }

    currentScreen =
        WIFI_RESULT;

    drawConnectionResult();
}

// ============================================================
// CONNECTION RESULT
// ============================================================

void drawConnectionResult()
{
    display.clearDisplay();

    if (
        WiFi.status() ==
        WL_CONNECTED
    )
    {
        drawHeader(
            "CONNECTED"
        );

        display.setCursor(
            2,
            17
        );

        String name =
            wifiNames[wifiIndex];

        if (name.length() > 18)
        {
            name =
                name.substring(
                    0,
                    18
                );
        }

        display.println(name);

        display.setCursor(
            2,
            30
        );

        display.print(
            "IP: "
        );

        display.println(
            WiFi.localIP()
        );

        display.setCursor(
            2,
            42
        );

        display.print(
            "RSSI: "
        );

        display.print(
            WiFi.RSSI()
        );

        display.println(
            " dBm"
        );

        display.setCursor(
            2,
            55
        );

        display.println(
            "A: Back"
        );
    }
    else
    {
        drawHeader(
            "FAILED"
        );

        display.setCursor(
            5,
            25
        );

        display.println(
            "Connection failed"
        );

        display.setCursor(
            5,
            38
        );

        display.println(
            "Check password"
        );

        display.setCursor(
            5,
            55
        );

        display.println(
            "A: Back"
        );
    }

    display.display();
}

// ============================================================
// NETWORK MENU
// ============================================================

void drawNetworkMenu()
{
    display.clearDisplay();

    drawHeader(
        "NETWORK"
    );

    for (
        int i = 0;
        i < NETWORK_MENU_COUNT;
        i++
    )
    {
        int y =
            18 + (i * 12);

        display.setCursor(
            5,
            y
        );

        if (
            i ==
            networkMenuIndex
        )
        {
            display.print("> ");
        }
        else
        {
            display.print("  ");
        }

        display.println(
            networkMenu[i]
        );
    }

    display.setCursor(
        5,
        56
    );

    display.println(
        "A: Scroll B: Enter"
    );

    display.display();
}

// ============================================================
// NETWORK INFO
// ============================================================

void drawNetworkInfo()
{
    display.clearDisplay();

    drawHeader(
        "NETWORK INFO"
    );

    if (
        WiFi.status() !=
        WL_CONNECTED
    )
    {
        display.setCursor(
            8,
            25
        );

        display.println(
            "Not connected"
        );

        display.setCursor(
            8,
            50
        );

        display.println(
            "A: Back"
        );

        display.display();

        return;
    }

    display.setCursor(
        2,
        14
    );

    display.print(
        "IP: "
    );

    display.println(
        WiFi.localIP()
    );

    display.setCursor(
        2,
        25
    );

    display.print(
        "GW: "
    );

    display.println(
        WiFi.gatewayIP()
    );

    display.setCursor(
        2,
        36
    );

    display.print(
        "SN: "
    );

    display.println(
        WiFi.subnetMask()
    );

    display.setCursor(
        2,
        47
    );

    display.print(
        "DNS: "
    );

    display.println(
        WiFi.dnsIP()
    );

    display.setCursor(
        2,
        58
    );

    display.print(
        "RSSI:"
    );

    display.print(
        WiFi.RSSI()
    );

    display.print(
        " dBm"
    );

    display.display();
}

// ============================================================
// LAN SCAN
// ============================================================

void startLANScan()
{
    if (
        WiFi.status() !=
        WL_CONNECTED
    )
    {
        currentScreen =
            LAN_SCAN;

        lanDeviceCount = 0;

        drawLANScan();

        return;
    }

    currentScreen =
        LAN_SCAN;

    lanDeviceCount = 0;

    display.clearDisplay();

    drawHeader(
        "LAN SCAN"
    );

    display.setCursor(
        10,
        25
    );

    display.println(
        "Scanning LAN..."
    );

    display.setCursor(
        10,
        40
    );

    display.println(
        "Please wait"
    );

    display.display();

    Serial.println();
    Serial.println(
        "=========================="
    );

    Serial.println(
        "LAN SCAN START"
    );

    Serial.println(
        "=========================="
    );

    IPAddress localIP =
        WiFi.localIP();

    IPAddress gateway =
        WiFi.gatewayIP();

    IPAddress subnet =
        WiFi.subnetMask();

    Serial.print(
        "Local IP: "
    );

    Serial.println(
        localIP
    );

    Serial.print(
        "Gateway: "
    );

    Serial.println(
        gateway
    );

    Serial.print(
        "Subnet: "
    );

    Serial.println(
        subnet
    );

    // --------------------------------------------------------
    // Basic /24 LAN scan
    // --------------------------------------------------------

    // This scanner assumes a typical home /24 network.
    // Example:
    // 192.168.1.x
    //
    // This keeps the scan practical on a tiny OLED device.
    // --------------------------------------------------------

    for (
        int host = 1;
        host <= 254;
        host++
    )
    {
        if (
            host ==
            localIP[3]
        )
        {
            continue;
        }

        IPAddress target(
            localIP[0],
            localIP[1],
            localIP[2],
            host
        );

        bool reachable =
            false;

        // Try common TCP services.
        // This catches many routers,
        // PCs, NAS devices, cameras, etc.

        int ports[] =
        {
            80,
            443,
            22,
            8080
        };

        for (
            int p = 0;
            p < 4;
            p++
        )
        {
            WiFiClient client;

            unsigned long t =
                millis();

            if (
                client.connect(
                    target,
                    ports[p],
                    80
                )
            )
            {
                reachable = true;

                client.stop();

                break;
            }

            client.stop();

            // Prevent one dead host
            // from consuming too much time.
            if (
                millis() - t > 100
            )
            {
                break;
            }
        }

        if (reachable)
        {
            if (
                lanDeviceCount <
                MAX_LAN_DEVICES
            )
            {
                lanDevices[
                    lanDeviceCount
                ] = target;

                lanDeviceCount++;

                Serial.print(
                    "FOUND: "
                );

                Serial.println(
                    target
                );
            }
        }

        // Update screen every 8 hosts
        if (
            host % 8 == 0
        )
        {
            display.clearDisplay();

            drawHeader(
                "LAN SCAN"
            );

            display.setCursor(
                10,
                25
            );

            display.print(
                "Scanning ."
            );

            display.print(
                host
            );

            display.print(
                "/254"
            );

            display.setCursor(
                10,
                40
            );

            display.print(
                "Found: "
            );

            display.println(
                lanDeviceCount
            );

            display.display();
        }

        delay(2);
    }

    Serial.print(
        "LAN devices found: "
    );

    Serial.println(
        lanDeviceCount
    );

    lanDeviceIndex = 0;

    currentScreen =
        LAN_LIST;

    drawLANList();
}

// ============================================================
// LAN SCAN SCREEN
// ============================================================

void drawLANScan()
{
    display.clearDisplay();

    drawHeader(
        "LAN SCAN"
    );

    if (
        WiFi.status() !=
        WL_CONNECTED
    )
    {
        display.setCursor(
            5,
            25
        );

        display.println(
            "Wi-Fi not connected"
        );

        display.setCursor(
            5,
            50
        );

        display.println(
            "A: Back"
        );
    }
    else
    {
        display.setCursor(
            10,
            28
        );

        display.println(
            "Scanning..."
        );
    }

    display.display();
}

// ============================================================
// LAN LIST
// ============================================================

void drawLANList()
{
    display.clearDisplay();

    drawHeader(
        "LAN DEVICES"
    );

    if (
        lanDeviceCount <= 0
    )
    {
        display.setCursor(
            8,
            25
        );

        display.println(
            "No devices found"
        );

        display.setCursor(
            8,
            50
        );

        display.println(
            "A: Back"
        );

        display.display();

        return;
    }

    int startIndex = 0;

    if (
        lanDeviceIndex >= 4
    )
    {
        startIndex =
            lanDeviceIndex - 3;
    }

    for (
        int row = 0;
        row < 4;
        row++
    )
    {
        int i =
            startIndex + row;

        if (
            i >= lanDeviceCount
        )
        {
            break;
        }

        int y =
            17 + (row * 11);

        display.setCursor(
            2,
            y
        );

        if (
            i ==
            lanDeviceIndex
        )
        {
            display.print(
                "> "
            );
        }
        else
        {
            display.print(
                "  "
            );
        }

        display.println(
            lanDevices[i]
        );
    }

    display.display();
}

// ============================================================
// LAN DEVICE INFO
// ============================================================

void drawLANDeviceInfo()
{
    display.clearDisplay();

    drawHeader(
        "DEVICE"
    );

    if (
        lanDeviceCount <= 0
    )
    {
        display.setCursor(
            5,
            25
        );

        display.println(
            "No device"
        );

        display.display();

        return;
    }

    display.setCursor(
        3,
        18
    );

    display.println(
        lanDevices[
            lanDeviceIndex
        ]
    );

    display.setCursor(
        3,
        31
    );

    display.println(
        "B: Ping"
    );

    display.setCursor(
        3,
        44
    );

    display.println(
        "A: Back"
    );

    display.display();
}

// ============================================================
// PING
// ============================================================

void pingSelectedDevice()
{
    currentScreen =
        PING_RESULT;

    display.clearDisplay();

    drawHeader(
        "PING"
    );

    if (
        lanDeviceCount <= 0
    )
    {
        display.setCursor(
            5,
            25
        );

        display.println(
            "No device"
        );

        display.display();

        return;
    }

    IPAddress target =
        lanDevices[
            lanDeviceIndex
        ];

    display.setCursor(
        3,
        18
    );

    display.println(
        target
    );

    display.setCursor(
        3,
        34
    );

    display.println(
        "Pinging..."
    );

    display.display();

    Serial.println();

    Serial.print(
        "PING "
    );

    Serial.println(
        target
    );

    bool success =
        Ping.ping(
            target,
            3
        );

    if (success)
    {
        float avg =
            Ping.averageTime();

        Serial.print(
            "Average: "
        );

        Serial.print(
            avg
        );

        Serial.println(
            " ms"
        );
    }
    else
    {
        Serial.println(
            "Ping failed"
        );
    }

    drawPingResult();
}

// ============================================================
// PING RESULT
// ============================================================

void drawPingResult()
{
    display.clearDisplay();

    drawHeader(
        "PING RESULT"
    );

    if (
        lanDeviceCount <= 0
    )
    {
        display.display();

        return;
    }

    display.setCursor(
        3,
        18
    );

    display.println(
        lanDevices[
            lanDeviceIndex
        ]
    );

    if (
        Ping.ping(
            lanDevices[
                lanDeviceIndex
            ],
            1
        )
    )
    {
        display.setCursor(
            3,
            32
        );

        display.println(
            "ONLINE"
        );

        display.setCursor(
            3,
            45
        );

        display.print(
            "Avg: "
        );

        display.print(
            Ping.averageTime()
        );

        display.println(
            " ms"
        );
    }
    else
    {
        display.setCursor(
            3,
            34
        );

        display.println(
            "NO RESPONSE"
        );
    }

    display.setCursor(
        3,
        56
    );

    display.println(
        "A: Back"
    );

    display.display();
}

// ============================================================
// PASSWORD MENU
// ============================================================

void drawPasswordMenu()
{
    display.clearDisplay();

    drawHeader(
        "PASSWORD"
    );

    display.setCursor(
        5,
        20
    );

    display.println(
        "> Generate"
    );

    display.setCursor(
        5,
        32
    );

    display.println(
        "  Length"
    );

    display.setCursor(
        5,
        44
    );

    display.print(
        "  "
    );

    display.print(
        passwordLengths[
            passwordLengthIndex
        ]
    );

    display.println(
        " characters"
    );

    display.setCursor(
        5,
        56
    );

    display.println(
        "B: Select"
    );

    display.display();
}

// ============================================================
// PASSWORD GENERATOR
// ============================================================

void generatePassword()
{
    generatedPassword = "";

    int length =
        passwordLengths[
            passwordLengthIndex
        ];

    int charsLength =
        strlen(passwordChars);

    for (
        int i = 0;
        i < length;
        i++
    )
    {
        uint32_t randomValue =
            esp_random();

        int index =
            randomValue %
            charsLength;

        generatedPassword +=
            passwordChars[index];
    }
}

// ============================================================
// PASSWORD GENERATOR SCREEN
// ============================================================

void drawPasswordGenerator()
{
    display.clearDisplay();

    drawHeader(
        "GENERATED PASSWORD"
    );

    display.setCursor(
        2,
        18
    );

    display.println(
        generatedPassword
    );

    display.setCursor(
        2,
        34
    );

    display.print(
        "Length: "
    );

    display.println(
        passwordLengths[
            passwordLengthIndex
        ]
    );

    display.setCursor(
        2,
        48
    );

    display.println(
        "B: New"
    );

    display.setCursor(
        70,
        48
    );

    display.println(
        "A: Back"
    );

    display.display();
}

// ============================================================
// STOPWATCH MENU
// ============================================================

void drawStopwatchMenu()
{
    display.clearDisplay();

    drawHeader(
        "STOPWATCH"
    );

    unsigned long elapsed;

    if (stopwatchRunning)
    {
        elapsed =
            millis() -
            stopwatchStart;
    }
    else
    {
        elapsed =
            stopwatchElapsed;
    }

    unsigned long totalSeconds =
        elapsed / 1000;

    unsigned int minutes =
        totalSeconds / 60;

    unsigned int seconds =
        totalSeconds % 60;

    char buffer[10];

    sprintf(
        buffer,
        "%02u:%02u",
        minutes,
        seconds
    );

    display.setTextSize(2);

    display.setCursor(
        28,
        22
    );

    display.println(
        buffer
    );

    display.setTextSize(1);

    display.setCursor(
        5,
        52
    );

    if (stopwatchRunning)
    {
        display.println(
            "B: Stop"
        );
    }
    else
    {
        display.println(
            "B: Start"
        );
    }

    display.setCursor(
        75,
        52
    );

    display.println(
        "A: Reset"
    );

    display.display();
}

// ============================================================
// STOPWATCH UPDATE
// ============================================================

void updateStopwatch()
{
    if (
        currentScreen !=
        STOPWATCH_MENU
    )
    {
        return;
    }

    static unsigned long lastDraw =
        0;

    if (
        millis() -
        lastDraw >=
        250
    )
    {
        lastDraw =
            millis();

        drawStopwatchMenu();
    }
}

// ============================================================
// ENTER MAIN MENU
// ============================================================

void enterMainMenu()
{
    switch (menuIndex)
    {
        case 0:
            currentScreen =
                WIFI_MENU;
            break;

        case 1:
            currentScreen =
                NETWORK_MENU;

            networkMenuIndex =
                0;
            break;

        case 2:
            currentScreen =
                PASSWORD_MENU;
            break;

        case 3:
            currentScreen =
                STOPWATCH_MENU;
            break;
    }

    drawScreen();
}

// ============================================================
// ENTER NETWORK MENU
// ============================================================

void enterNetworkMenu()
{
    switch (
        networkMenuIndex
    )
    {
        case 0:

            currentScreen =
                NETWORK_INFO;

            break;

        case 1:

            startLANScan();

            break;

        case 2:

            if (
                lanDeviceCount > 0
            )
            {
                currentScreen =
                    LAN_LIST;
            }
            else
            {
                startLANScan();
            }

            break;
    }

    drawScreen();
}

// ============================================================
// ENTER PASSWORD MENU
// ============================================================

void enterPasswordMenu()
{
    currentScreen =
        PASSWORD_GENERATOR;

    generatePassword();

    drawPasswordGenerator();
}

// ============================================================
// GO BACK
// ============================================================

void goBack()
{
    switch (
        currentScreen
    )
    {
        case MAIN_MENU:
            break;

        case WIFI_MENU:
            currentScreen =
                MAIN_MENU;

            menuIndex = 0;
            break;

        case WIFI_SCAN:
            currentScreen =
                WIFI_MENU;
            break;

        case WIFI_SELECTED:
            currentScreen =
                WIFI_SCAN;
            break;

        case WIFI_PASSWORD:
            currentScreen =
                WIFI_SELECTED;
            break;

        case WIFI_CONNECTING:
            break;

        case WIFI_RESULT:
            currentScreen =
                WIFI_SELECTED;
            break;

        case NETWORK_MENU:
            currentScreen =
                MAIN_MENU;

            menuIndex = 1;
            break;

        case NETWORK_INFO:
            currentScreen =
                NETWORK_MENU;
            break;

        case LAN_SCAN:
            currentScreen =
                NETWORK_MENU;
            break;

        case LAN_LIST:
            currentScreen =
                NETWORK_MENU;
            break;

        case LAN_DEVICE_INFO:
            currentScreen =
                LAN_LIST;
            break;

        case PING_RESULT:
            currentScreen =
                LAN_DEVICE_INFO;
            break;

        case PASSWORD_MENU:
            currentScreen =
                MAIN_MENU;

            menuIndex = 2;
            break;

        case PASSWORD_GENERATOR:
            currentScreen =
                PASSWORD_MENU;
            break;

        case STOPWATCH_MENU:
            currentScreen =
                MAIN_MENU;

            menuIndex = 3;
            break;
    }

    drawScreen();
}

// ============================================================
// DRAW SCREEN
// ============================================================

void drawScreen()
{
    switch (
        currentScreen
    )
    {
        case MAIN_MENU:
            drawMainMenu();
            break;

        case WIFI_MENU:
            drawWiFiMenu();
            break;

        case WIFI_SCAN:
            drawWiFiList();
            break;

        case WIFI_SELECTED:
            drawSelectedWiFi();
            break;

        case WIFI_PASSWORD:
            drawPasswordKeyboard();
            break;

        case WIFI_CONNECTING:
            break;

        case WIFI_RESULT:
            drawConnectionResult();
            break;

        case NETWORK_MENU:
            drawNetworkMenu();
            break;

        case NETWORK_INFO:
            drawNetworkInfo();
            break;

        case LAN_SCAN:
            drawLANScan();
            break;

        case LAN_LIST:
            drawLANList();
            break;

        case LAN_DEVICE_INFO:
            drawLANDeviceInfo();
            break;

        case PING_RESULT:
            drawPingResult();
            break;

        case PASSWORD_MENU:
            drawPasswordMenu();
            break;

        case PASSWORD_GENERATOR:
            drawPasswordGenerator();
            break;

        case STOPWATCH_MENU:
            drawStopwatchMenu();
            break;
    }
}

// ============================================================
// SETUP
// ============================================================

void setup()
{
    Serial.begin(
        115200
    );

    delay(500);

    pinMode(
        BUTTON_A,
        INPUT_PULLUP
    );

    pinMode(
        BUTTON_B,
        INPUT_PULLUP
    );

    // --------------------------------------------------------
    // ESP32-C3 Wi-Fi stability
    // --------------------------------------------------------

    WiFi.persistent(false);

    WiFi.disconnect(
        true,
        true
    );

    delay(100);

    WiFi.mode(
        WIFI_STA
    );

    WiFi.setTxPower(
        WIFI_POWER_11dBm
    );

    WiFi.setSleep(false);

    // --------------------------------------------------------
    // OLED
    // --------------------------------------------------------

    Wire.begin(
        OLED_SDA,
        OLED_SCL
    );

    if (
        !display.begin(
            SSD1306_SWITCHCAPVCC,
            OLED_ADDR
        )
    )
    {
        Serial.println(
            "OLED ERROR"
        );

        while (true)
        {
            delay(1000);
        }
    }

    Serial.println();
    Serial.println(
        "=============================="
    );

    Serial.println(
        "   POCKET CYBER DECK v0.4"
    );

    Serial.println(
        "=============================="
    );

    Serial.println(
        "OLED: OK"
    );

    Serial.println(
        "Wi-Fi: STABILITY MODE"
    );

    Serial.println(
        "LAN: READY"
    );

    Serial.println(
        "PASSWORD: READY"
    );

    Serial.println(
        "STOPWATCH: READY"
    );

    drawScreen();
}

// ============================================================
// LOOP
// ============================================================

void loop()
{
    // ========================================================
    // A SHORT
    // ========================================================

    if (
        buttonPressed(buttonA)
    )
    {
        // Main Menu
        if (
            currentScreen ==
            MAIN_MENU
        )
        {
            menuIndex++;

            if (
                menuIndex >=
                MAIN_MENU_COUNT
            )
            {
                menuIndex = 0;
            }

            drawMainMenu();
        }

        // Wi-Fi List
        else if (
            currentScreen ==
            WIFI_SCAN
        )
        {
            if (
                wifiCount > 0
            )
            {
                wifiIndex++;

                if (
                    wifiIndex >=
                    wifiCount
                )
                {
                    wifiIndex = 0;
                }

                drawWiFiList();
            }
        }

        // Password keyboard
        else if (
            currentScreen ==
            WIFI_PASSWORD
        )
        {
            const char* chars =
                passwordSets[
                    passwordSet
                ];

            int length =
                strlen(chars);

            passwordIndex++;

            if (
                passwordIndex >=
                length
            )
            {
                passwordIndex = 0;
            }

            drawPasswordKeyboard();
        }

        // Network menu
        else if (
            currentScreen ==
            NETWORK_MENU
        )
        {
            networkMenuIndex++;

            if (
                networkMenuIndex >=
                NETWORK_MENU_COUNT
            )
            {
                networkMenuIndex = 0;
            }

            drawNetworkMenu();
        }

        // LAN list
        else if (
            currentScreen ==
            LAN_LIST
        )
        {
            if (
                lanDeviceCount > 0
            )
            {
                lanDeviceIndex++;

                if (
                    lanDeviceIndex >=
                    lanDeviceCount
                )
                {
                    lanDeviceIndex = 0;
                }

                drawLANList();
            }
        }

        // Password generator
        else if (
            currentScreen ==
            PASSWORD_GENERATOR
        )
        {
            // Change password length
            passwordLengthIndex++;

            if (
                passwordLengthIndex >= 3
            )
            {
                passwordLengthIndex = 0;
            }

            generatePassword();

            drawPasswordGenerator();
        }

        // Stopwatch
        else if (
            currentScreen ==
            STOPWATCH_MENU
        )
        {
            stopwatchElapsed = 0;

            if (stopwatchRunning)
            {
                stopwatchStart =
                    millis();
            }

            drawStopwatchMenu();
        }
    }

    // ========================================================
    // A LONG
    // ========================================================

    if (
        buttonLongPressed(buttonA)
    )
    {
        if (
            currentScreen ==
            WIFI_PASSWORD
        )
        {
            passwordSet++;

            if (
                passwordSet >=
                PASSWORD_SET_COUNT
            )
            {
                passwordSet = 0;
            }

            passwordIndex = 0;

            drawPasswordKeyboard();
        }
        else
        {
            goBack();
        }
    }

    // ========================================================
    // B SHORT
    // ========================================================

    if (
        buttonPressed(buttonB)
    )
    {
        // Main menu
        if (
            currentScreen ==
            MAIN_MENU
        )
        {
            enterMainMenu();
        }

        // Wi-Fi menu
        else if (
            currentScreen ==
            WIFI_MENU
        )
        {
            startWiFiScan();
        }

        // Wi-Fi list
        else if (
            currentScreen ==
            WIFI_SCAN
        )
        {
            if (
                wifiCount > 0
            )
            {
                currentScreen =
                    WIFI_SELECTED;

                drawSelectedWiFi();
            }
        }

        // Selected Wi-Fi
        else if (
            currentScreen ==
            WIFI_SELECTED
        )
        {
            wifiPassword = "";

            passwordSet = 0;

            passwordIndex = 0;

            currentScreen =
                WIFI_PASSWORD;

            drawPasswordKeyboard();
        }

        // Password keyboard
        else if (
            currentScreen ==
            WIFI_PASSWORD
        )
        {
            const char* chars =
                passwordSets[
                    passwordSet
                ];

            char selected =
                chars[
                    passwordIndex
                ];

            // # = Connect
            if (
                passwordSet == 3 &&
                selected == '#'
            )
            {
                if (
                    wifiPassword.length() >
                    0
                )
                {
                    startWiFiConnection();
                }
            }
            else
            {
                wifiPassword +=
                    selected;

                passwordIndex = 0;

                drawPasswordKeyboard();
            }
        }

        // Network menu
        else if (
            currentScreen ==
            NETWORK_MENU
        )
        {
            enterNetworkMenu();
        }

        // LAN list
        else if (
            currentScreen ==
            LAN_LIST
        )
        {
            if (
                lanDeviceCount > 0
            )
            {
                currentScreen =
                    LAN_DEVICE_INFO;

                drawLANDeviceInfo();
            }
        }

        // LAN Device
        else if (
            currentScreen ==
            LAN_DEVICE_INFO
        )
        {
            pingSelectedDevice();
        }

        // Password menu
        else if (
            currentScreen ==
            PASSWORD_MENU
        )
        {
            enterPasswordMenu();
        }

        // Password generator
        else if (
            currentScreen ==
            PASSWORD_GENERATOR
        )
        {
            generatePassword();

            drawPasswordGenerator();
        }

        // Stopwatch
        else if (
            currentScreen ==
            STOPWATCH_MENU
        )
        {
            if (
                stopwatchRunning
            )
            {
                stopwatchElapsed =
                    millis() -
                    stopwatchStart;

                stopwatchRunning =
                    false;
            }
            else
            {
                stopwatchStart =
                    millis() -
                    stopwatchElapsed;

                stopwatchRunning =
                    true;
            }

            drawStopwatchMenu();
        }
    }

    // ========================================================
    // B LONG
    // ========================================================

    if (
        buttonLongPressed(buttonB)
    )
    {
        // Password keyboard
        if (
            currentScreen ==
            WIFI_PASSWORD
        )
        {
            if (
                wifiPassword.length() >
                0
            )
            {
                wifiPassword.remove(
                    wifiPassword.length() - 1
                );
            }

            drawPasswordKeyboard();
        }

        // Stopwatch reset
        else if (
            currentScreen ==
            STOPWATCH_MENU
        )
        {
            stopwatchRunning =
                false;

            stopwatchElapsed =
                0;

            drawStopwatchMenu();
        }
    }

    // ========================================================
    // STOPWATCH UPDATE
    // ========================================================

    updateStopwatch();

    delay(5);
}