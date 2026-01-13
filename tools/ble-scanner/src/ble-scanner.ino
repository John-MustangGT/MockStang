/**
 * BLE Scanner for ELM327 Adapters
 *
 * Hardware: Adafruit Feather ESP32-S3 Reverse TFT
 *
 * This tool discovers and enumerates GATT services/characteristics
 * from BLE OBD-II adapters (Vgate iCar, OBD-II, etc.)
 *
 * Buttons:
 *  - Button A (GPIO 9): Start/Stop Scan
 *  - Button B (GPIO 6): Connect to selected device
 *  - Button C (GPIO 5): Next device in list
 *
 * Display: ST7789 135x240 TFT
 */

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// ==================== Hardware Configuration ====================

// TFT Display pins for Adafruit Feather ESP32-S3 Reverse TFT
#define TFT_CS         7
#define TFT_RST        40
#define TFT_DC         39
#define TFT_BACKLIGHT  45

// Button pins
#define BUTTON_A       9   // Left button
#define BUTTON_B       6   // Middle button
#define BUTTON_C       5   // Right button

// Display configuration
#define TFT_WIDTH      135
#define TFT_HEIGHT     240

// Colors (RGB565)
#define COLOR_BG       0x0000  // Black
#define COLOR_TEXT     0xFFFF  // White
#define COLOR_HEADER   0x07FF  // Cyan
#define COLOR_SUCCESS  0x07E0  // Green
#define COLOR_ERROR    0xF800  // Red
#define COLOR_WARNING  0xFFE0  // Yellow

// ==================== Global Objects ====================

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
NimBLEScan* pBLEScan;

// ==================== Scanner State ====================

struct BLEDeviceInfo {
    String name;
    String address;
    int rssi;
    bool isTarget;
};

std::vector<BLEDeviceInfo> discoveredDevices;
int selectedDeviceIndex = 0;
bool isScanning = false;
bool isConnected = false;
NimBLEClient* pClient = nullptr;
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 200;

// ==================== Display Functions ====================

void displayHeader(const char* title) {
    tft.fillRect(0, 0, TFT_WIDTH, 20, COLOR_HEADER);
    tft.setTextColor(COLOR_BG);
    tft.setTextSize(1);
    tft.setCursor(2, 6);
    tft.print(title);
}

void displayStatus(const char* status, uint16_t color = COLOR_TEXT) {
    tft.fillRect(0, TFT_HEIGHT - 20, TFT_WIDTH, 20, COLOR_BG);
    tft.setTextColor(color);
    tft.setTextSize(1);
    tft.setCursor(2, TFT_HEIGHT - 16);
    tft.print(status);
}

void displayDeviceList() {
    tft.fillRect(0, 22, TFT_WIDTH, TFT_HEIGHT - 44, COLOR_BG);
    tft.setTextSize(1);

    int y = 24;
    int maxVisible = 10;
    int startIdx = max(0, selectedDeviceIndex - maxVisible + 1);

    for (int i = startIdx; i < discoveredDevices.size() && i < startIdx + maxVisible; i++) {
        if (y > TFT_HEIGHT - 24) break;

        bool isSelected = (i == selectedDeviceIndex);
        uint16_t textColor = discoveredDevices[i].isTarget ? COLOR_SUCCESS : COLOR_TEXT;

        if (isSelected) {
            tft.fillRect(0, y - 2, TFT_WIDTH, 12, 0x2104); // Dark blue highlight
        }

        tft.setTextColor(textColor);
        tft.setCursor(2, y);

        String displayName = discoveredDevices[i].name;
        if (displayName.length() > 18) {
            displayName = displayName.substring(0, 18);
        }

        tft.print(displayName);
        tft.print(" ");
        tft.print(discoveredDevices[i].rssi);

        y += 12;
    }

    // Display device count
    char countStr[32];
    snprintf(countStr, sizeof(countStr), "%d devices (%d targets)",
             discoveredDevices.size(),
             std::count_if(discoveredDevices.begin(), discoveredDevices.end(),
                          [](const BLEDeviceInfo& d) { return d.isTarget; }));
    displayStatus(countStr, COLOR_WARNING);
}

void displayConnectionInfo(const char* deviceName) {
    tft.fillRect(0, 22, TFT_WIDTH, TFT_HEIGHT - 44, COLOR_BG);
    tft.setTextColor(COLOR_SUCCESS);
    tft.setTextSize(1);
    tft.setCursor(2, 30);
    tft.print("Connected:");
    tft.setCursor(2, 42);
    tft.setTextColor(COLOR_TEXT);
    tft.print(deviceName);
    displayStatus("Enumerating...", COLOR_WARNING);
}

void clearScreen() {
    tft.fillScreen(COLOR_BG);
}

// ==================== BLE Callback Classes ====================

class MyAdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        String deviceName = advertisedDevice->getName().c_str();
        String deviceAddr = advertisedDevice->getAddress().toString().c_str();

        // Check if already in list
        for (const auto& dev : discoveredDevices) {
            if (dev.address == deviceAddr) {
                return; // Already found
            }
        }

        // Check if this is a target device (ELM327/OBD-II adapter)
        bool isTarget = false;
        String nameLower = deviceName;
        nameLower.toLowerCase();

        if (nameLower.indexOf("icar") >= 0 ||
            nameLower.indexOf("vgate") >= 0 ||
            nameLower.indexOf("obd") >= 0 ||
            nameLower.indexOf("elm") >= 0 ||
            nameLower.indexOf("obdii") >= 0) {
            isTarget = true;
        }

        BLEDeviceInfo info;
        info.name = deviceName.length() > 0 ? deviceName : "(Unknown)";
        info.address = deviceAddr;
        info.rssi = advertisedDevice->getRSSI();
        info.isTarget = isTarget;

        discoveredDevices.push_back(info);

        Serial.println("=========================================");
        Serial.printf("Found Device: %s\n", info.name.c_str());
        Serial.printf("  Address: %s\n", info.address.c_str());
        Serial.printf("  RSSI: %d dBm\n", info.rssi);
        Serial.printf("  Target: %s\n", isTarget ? "YES" : "no");

        if (advertisedDevice->haveServiceUUID()) {
            Serial.printf("  Service UUID: %s\n",
                         advertisedDevice->getServiceUUID().toString().c_str());
        }

        Serial.println("=========================================");

        displayDeviceList();
    }
};

// ==================== BLE Functions ====================

void printCharacteristicProperties(NimBLERemoteCharacteristic* pChar) {
    Serial.print("    Properties: ");

    if (pChar->canRead()) Serial.print("READ ");
    if (pChar->canWrite()) Serial.print("WRITE ");
    if (pChar->canWriteNoResponse()) Serial.print("WRITE_NR ");
    if (pChar->canNotify()) Serial.print("NOTIFY ");
    if (pChar->canIndicate()) Serial.print("INDICATE ");
    if (pChar->canBroadcast()) Serial.print("BROADCAST ");

    Serial.println();
}

void enumerateServices(NimBLEClient* pClient) {
    Serial.println("\n╔════════════════════════════════════════════════════════════════╗");
    Serial.println("║          GATT SERVICE AND CHARACTERISTIC ENUMERATION           ║");
    Serial.println("╚════════════════════════════════════════════════════════════════╝\n");

    std::vector<NimBLERemoteService*>* pServices = pClient->getServices(true);

    if (pServices == nullptr) {
        Serial.println("ERROR: Failed to get services!");
        return;
    }

    Serial.printf("Found %d services\n\n", pServices->size());

    for (auto pService : *pServices) {
        Serial.println("┌────────────────────────────────────────────────────────────────┐");
        Serial.printf("│ SERVICE: %s\n", pService->getUUID().toString().c_str());
        Serial.println("├────────────────────────────────────────────────────────────────┤");

        // Check for common service types
        String serviceUUID = pService->getUUID().toString().c_str();
        if (serviceUUID == "0000fff0-0000-1000-8000-00805f9b34fb") {
            Serial.println("│ ⭐ LIKELY ELM327 SERVICE (Custom UUID)");
        } else if (serviceUUID == "e7810a71-73ae-499d-8c15-faa9aef0c3f2") {
            Serial.println("│ ⭐ Nordic UART Service");
        } else if (serviceUUID.startsWith("0000180")) {
            Serial.println("│ ℹ️  Standard Bluetooth Service");
        }

        std::vector<NimBLERemoteCharacteristic*>* pCharacteristics = pService->getCharacteristics(true);

        if (pCharacteristics != nullptr) {
            Serial.printf("│ Characteristics: %d\n", pCharacteristics->size());
            Serial.println("│");

            for (auto pChar : *pCharacteristics) {
                Serial.printf("│  └─ CHARACTERISTIC: %s\n", pChar->getUUID().toString().c_str());

                // Check for common characteristic types
                String charUUID = pChar->getUUID().toString().c_str();
                if (charUUID == "0000fff1-0000-1000-8000-00805f9b34fb") {
                    Serial.println("│      ⭐ LIKELY TX (Write to device)");
                } else if (charUUID == "0000fff2-0000-1000-8000-00805f9b34fb") {
                    Serial.println("│      ⭐ LIKELY RX (Read from device/Notify)");
                }

                Serial.print("│      ");
                printCharacteristicProperties(pChar);

                // Try to read value if readable
                if (pChar->canRead()) {
                    try {
                        std::string value = pChar->readValue();
                        if (value.length() > 0 && value.length() < 100) {
                            Serial.printf("│      Value: %s (hex: ", value.c_str());
                            for (char c : value) {
                                Serial.printf("%02X ", (uint8_t)c);
                            }
                            Serial.println(")");
                        }
                    } catch (...) {
                        Serial.println("│      (Read failed)");
                    }
                }

                // Enumerate descriptors
                std::vector<NimBLERemoteDescriptor*>* pDescriptors = pChar->getDescriptors(true);
                if (pDescriptors != nullptr && pDescriptors->size() > 0) {
                    Serial.println("│      Descriptors:");
                    for (auto pDesc : *pDescriptors) {
                        Serial.printf("│        • %s\n", pDesc->getUUID().toString().c_str());
                    }
                }

                Serial.println("│");
            }
        }

        Serial.println("└────────────────────────────────────────────────────────────────┘\n");
    }

    Serial.println("\n╔════════════════════════════════════════════════════════════════╗");
    Serial.println("║                     ENUMERATION COMPLETE                       ║");
    Serial.println("╚════════════════════════════════════════════════════════════════╝\n");
}

bool connectToDevice(int index) {
    if (index < 0 || index >= discoveredDevices.size()) {
        return false;
    }

    BLEDeviceInfo& device = discoveredDevices[index];

    Serial.println("\n╔════════════════════════════════════════════════════════════════╗");
    Serial.printf("║ Connecting to: %-47s ║\n", device.name.c_str());
    Serial.printf("║ Address: %-54s ║\n", device.address.c_str());
    Serial.println("╚════════════════════════════════════════════════════════════════╝\n");

    displayConnectionInfo(device.name.c_str());

    NimBLEAddress bleAddress(device.address.c_str());

    if (pClient == nullptr) {
        pClient = NimBLEDevice::createClient();
    }

    if (!pClient->connect(bleAddress, true)) {
        Serial.println("ERROR: Failed to connect!");
        displayStatus("Connect failed!", COLOR_ERROR);
        return false;
    }

    Serial.println("✓ Connected successfully!");
    Serial.printf("MTU: %d bytes\n", pClient->getMTU());
    Serial.println();

    isConnected = true;

    // Enumerate all services and characteristics
    enumerateServices(pClient);

    displayStatus("Enumeration done!", COLOR_SUCCESS);

    return true;
}

void disconnectDevice() {
    if (pClient != nullptr && pClient->isConnected()) {
        pClient->disconnect();
        Serial.println("Disconnected from device");
    }
    isConnected = false;
}

void startScan() {
    discoveredDevices.clear();
    selectedDeviceIndex = 0;

    clearScreen();
    displayHeader("BLE Scanner");
    displayStatus("Scanning...", COLOR_WARNING);

    Serial.println("\n╔════════════════════════════════════════════════════════════════╗");
    Serial.println("║                    STARTING BLE SCAN                           ║");
    Serial.println("║  Looking for: iCar, Vgate, OBD, ELM327, OBDII                  ║");
    Serial.println("╚════════════════════════════════════════════════════════════════╝\n");

    pBLEScan->start(10, false); // 10 second scan

    isScanning = false;
    displayStatus("Scan complete", COLOR_SUCCESS);

    Serial.println("\n╔════════════════════════════════════════════════════════════════╗");
    Serial.println("║                    SCAN COMPLETE                               ║");
    Serial.printf("║  Found %d devices total                                         ║\n",
                  discoveredDevices.size());
    Serial.printf("║  Found %d target devices                                        ║\n",
                  (int)std::count_if(discoveredDevices.begin(), discoveredDevices.end(),
                                    [](const BLEDeviceInfo& d) { return d.isTarget; }));
    Serial.println("╚════════════════════════════════════════════════════════════════╝\n");
}

// ==================== Button Handlers ====================

void handleButtons() {
    unsigned long now = millis();
    if (now - lastButtonPress < debounceDelay) {
        return;
    }

    // Button A - Start/Stop Scan
    if (digitalRead(BUTTON_A) == LOW) {
        lastButtonPress = now;

        if (isConnected) {
            disconnectDevice();
            clearScreen();
            displayHeader("BLE Scanner");
            displayDeviceList();
        } else if (!isScanning) {
            isScanning = true;
            startScan();
        }
    }

    // Button B - Connect to selected device
    if (digitalRead(BUTTON_B) == LOW) {
        lastButtonPress = now;

        if (!isScanning && discoveredDevices.size() > 0 && !isConnected) {
            connectToDevice(selectedDeviceIndex);
        }
    }

    // Button C - Next device
    if (digitalRead(BUTTON_C) == LOW) {
        lastButtonPress = now;

        if (!isScanning && discoveredDevices.size() > 0 && !isConnected) {
            selectedDeviceIndex = (selectedDeviceIndex + 1) % discoveredDevices.size();
            displayDeviceList();

            Serial.printf("Selected: %s (%s)\n",
                         discoveredDevices[selectedDeviceIndex].name.c_str(),
                         discoveredDevices[selectedDeviceIndex].address.c_str());
        }
    }
}

// ==================== Setup & Loop ====================

void setup() {
    // Initialize serial
    Serial.begin(115200);
    delay(1000); // Wait for USB CDC

    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════════════════════════════╗");
    Serial.println("║                                                                ║");
    Serial.println("║          BLE Scanner for ELM327 OBD-II Adapters                ║");
    Serial.println("║                                                                ║");
    Serial.println("║  Hardware: Adafruit Feather ESP32-S3 Reverse TFT              ║");
    Serial.println("║  Purpose: Discover GATT services/characteristics              ║");
    Serial.println("║                                                                ║");
    Serial.println("╚════════════════════════════════════════════════════════════════╝");
    Serial.println();

    // Initialize buttons
    pinMode(BUTTON_A, INPUT_PULLUP);
    pinMode(BUTTON_B, INPUT_PULLUP);
    pinMode(BUTTON_C, INPUT_PULLUP);

    // Initialize TFT display
    pinMode(TFT_BACKLIGHT, OUTPUT);
    digitalWrite(TFT_BACKLIGHT, HIGH);

    tft.init(TFT_WIDTH, TFT_HEIGHT);
    tft.setRotation(3); // Landscape
    tft.fillScreen(COLOR_BG);

    displayHeader("BLE Scanner");

    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(1);
    tft.setCursor(2, 30);
    tft.println("Initializing...");

    // Initialize NimBLE
    Serial.println("Initializing NimBLE...");
    NimBLEDevice::init("ELM327-Scanner");

    // Create BLE Scanner
    pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), false);
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);

    Serial.println("✓ BLE initialized");
    Serial.println();
    Serial.println("Controls:");
    Serial.println("  Button A: Start/Stop Scan");
    Serial.println("  Button B: Connect to device");
    Serial.println("  Button C: Next device");
    Serial.println();
    Serial.println("Ready! Press Button A to start scanning...");
    Serial.println();

    tft.fillRect(0, 30, TFT_WIDTH, 40, COLOR_BG);
    tft.setCursor(2, 30);
    tft.println("Ready!");
    tft.setCursor(2, 50);
    tft.setTextSize(1);
    tft.println("A: Scan");
    tft.setCursor(2, 62);
    tft.println("B: Connect");
    tft.setCursor(2, 74);
    tft.println("C: Next");

    displayStatus("Press A to scan", COLOR_WARNING);
}

void loop() {
    handleButtons();
    delay(10);
}
