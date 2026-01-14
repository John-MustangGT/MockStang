/**
 * MockStang BLE Sniffer - ESP32-S3 BLE Client
 *
 * Connects to real Vgate/Vlinker "IOS-Vlink" adapter and logs all communication
 * Use this to capture exact byte sequences from the real adapter for comparison
 *
 * Build with: pio run -e esp32s3_sniffer --target upload
 * Monitor with: pio device monitor -e esp32s3_sniffer
 */

#include <Arduino.h>
#include <NimBLEDevice.h>

// Target device to connect to
#define TARGET_DEVICE_NAME "IOS-Vlink"

// Service and characteristic UUIDs to test
#define OBD_SERVICE_UUID "E7810A71-73AE-499D-8C15-FAA9AEF0C3F2"
#define OBD_CHAR_UUID "BEF8D6C9-9C21-4C9E-B632-BD58C1009F9F"

// Global state
static NimBLEClient* pClient = nullptr;
static NimBLERemoteCharacteristic* pOBDCharacteristic = nullptr;
static NimBLERemoteCharacteristic* pCustomWriteChar = nullptr;
static NimBLERemoteCharacteristic* pCustomNotifyChar = nullptr;
static bool doConnect = false;
static bool connected = false;
static bool servicesDiscovered = false;
static NimBLEAdvertisedDevice* targetDevice = nullptr;

// AT commands to test
const char* testCommands[] = {
    "ATD\r",
    "ATZ\r",
    "ATI\r",
    "AT@1\r",
    "AT@2\r",
    "ATRV\r",
    "ATSP0\r",
    "0100\r",
    nullptr
};

// Helper: Print hex dump of data
void hexDump(const char* label, const uint8_t* data, size_t len) {
    Serial.printf("%s (%d bytes): ", label, len);
    for (size_t i = 0; i < len; i++) {
        Serial.printf("0x%02X ", data[i]);
    }
    Serial.print(" | ASCII: ");
    for (size_t i = 0; i < len; i++) {
        if (data[i] >= 32 && data[i] < 127) {
            Serial.print((char)data[i]);
        } else {
            Serial.printf("[%02X]", data[i]);
        }
    }
    Serial.println();
}

// Notification callback for OBD characteristic
static void notifyCallback(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
    Serial.println("\n>>> NOTIFICATION RECEIVED <<<");
    Serial.printf("Characteristic: %s\n", pChar->getUUID().toString().c_str());
    hexDump("Data", pData, length);
}

// Notification callback for Custom Service 0x2AF0
static void customNotifyCallback(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
    Serial.println("\n>>> CUSTOM SERVICE NOTIFICATION <<<");
    Serial.printf("Characteristic: %s\n", pChar->getUUID().toString().c_str());
    hexDump("Data", pData, length);
}

// Client callback for connection events
class ClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) {
        Serial.println("\n=== CONNECTED TO DEVICE ===");
        connected = true;
    }

    void onDisconnect(NimBLEClient* pClient) {
        Serial.println("\n=== DISCONNECTED FROM DEVICE ===");
        connected = false;
        servicesDiscovered = false;
        Serial.println("\nRestarting scan in 5 seconds...");
    }
};

// Scan callback
class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        Serial.printf("Found device: %s", advertisedDevice->getName().c_str());

        if (advertisedDevice->haveName() && advertisedDevice->getName() == TARGET_DEVICE_NAME) {
            Serial.println(" <<< TARGET FOUND!");
            Serial.printf("  Address: %s\n", advertisedDevice->getAddress().toString().c_str());
            Serial.printf("  RSSI: %d dBm\n", advertisedDevice->getRSSI());

            // Show advertised services
            if (advertisedDevice->haveServiceUUID()) {
                Serial.println("  Advertised Services:");
                for (int i = 0; i < advertisedDevice->getServiceUUIDCount(); i++) {
                    Serial.printf("    - %s\n", advertisedDevice->getServiceUUID(i).toString().c_str());
                }
            }

            NimBLEDevice::getScan()->stop();
            targetDevice = advertisedDevice;
            doConnect = true;
        } else {
            Serial.println();
        }
    }
};

bool connectToServer() {
    Serial.println("\n=== ATTEMPTING CONNECTION ===");

    pClient = NimBLEDevice::createClient();
    pClient->setClientCallbacks(new ClientCallbacks(), false);
    pClient->setConnectTimeout(5);

    if (!pClient->connect(targetDevice)) {
        Serial.println("ERROR: Failed to connect");
        NimBLEDevice::deleteClient(pClient);
        pClient = nullptr;
        return false;
    }

    Serial.println("Connected! Discovering services...");

    // Get all services
    std::vector<NimBLERemoteService*>* pServices = pClient->getServices(true);
    if (pServices) {
        Serial.printf("\nFound %d services:\n", pServices->size());
        for (auto pService : *pServices) {
            Serial.printf("\n  Service: %s\n", pService->getUUID().toString().c_str());

            // Get all characteristics for this service
            std::vector<NimBLERemoteCharacteristic*>* pChars = pService->getCharacteristics(true);
            if (pChars) {
                for (auto pChar : *pChars) {
                    Serial.printf("    Characteristic: %s\n", pChar->getUUID().toString().c_str());
                    Serial.print("      Properties: ");
                    if (pChar->canRead()) Serial.print("Read ");
                    if (pChar->canWrite()) Serial.print("Write ");
                    if (pChar->canWriteNoResponse()) Serial.print("Write_NR ");
                    if (pChar->canNotify()) Serial.print("Notify ");
                    if (pChar->canIndicate()) Serial.print("Indicate ");
                    Serial.println();

                    // Store important characteristics
                    if (pChar->getUUID() == NimBLEUUID(OBD_CHAR_UUID)) {
                        pOBDCharacteristic = pChar;
                        Serial.println("      ^^^ OBD CHARACTERISTIC FOUND!");
                    }

                    // Custom Service 18F0 characteristics
                    if (pService->getUUID() == NimBLEUUID((uint16_t)0x18F0)) {
                        if (pChar->getUUID() == NimBLEUUID((uint16_t)0x2AF1)) {
                            pCustomWriteChar = pChar;
                            Serial.println("      ^^^ CUSTOM WRITE CHARACTERISTIC (0x2AF1)");
                        }
                        if (pChar->getUUID() == NimBLEUUID((uint16_t)0x2AF0)) {
                            pCustomNotifyChar = pChar;
                            Serial.println("      ^^^ CUSTOM NOTIFY CHARACTERISTIC (0x2AF0)");
                        }
                    }
                }
            }
        }
    }

    // Subscribe to notifications on both characteristics
    Serial.println("\n=== SUBSCRIBING TO NOTIFICATIONS ===");

    if (pOBDCharacteristic && pOBDCharacteristic->canNotify()) {
        Serial.println("Subscribing to OBD characteristic...");
        if (pOBDCharacteristic->subscribe(true, notifyCallback)) {
            Serial.println("  ✓ Subscribed to OBD notifications");
        } else {
            Serial.println("  ✗ Failed to subscribe to OBD");
        }
    }

    if (pCustomNotifyChar && pCustomNotifyChar->canNotify()) {
        Serial.println("Subscribing to Custom Service 0x2AF0...");
        if (pCustomNotifyChar->subscribe(true, customNotifyCallback)) {
            Serial.println("  ✓ Subscribed to Custom Service notifications");
        } else {
            Serial.println("  ✗ Failed to subscribe to Custom Service");
        }
    }

    Serial.println("\n=== SERVICE DISCOVERY COMPLETE ===");
    servicesDiscovered = true;
    return true;
}

void sendCommand(const char* cmd) {
    size_t len = strlen(cmd);
    Serial.printf("\n>>> SENDING COMMAND <<<\n");
    hexDump("Command", (const uint8_t*)cmd, len);

    bool sent = false;

    // Try Custom Service first (this is what OBD apps seem to prefer)
    if (pCustomWriteChar && pCustomWriteChar->canWrite()) {
        Serial.println("Writing to Custom Service 0x2AF1...");
        if (pCustomWriteChar->writeValue((uint8_t*)cmd, len, false)) {
            Serial.println("  ✓ Written to Custom Service");
            sent = true;
        } else {
            Serial.println("  ✗ Failed to write to Custom Service");
        }
    }

    // Also try main OBD characteristic
    if (pOBDCharacteristic && pOBDCharacteristic->canWrite()) {
        Serial.println("Writing to OBD characteristic...");
        if (pOBDCharacteristic->writeValue((uint8_t*)cmd, len, false)) {
            Serial.println("  ✓ Written to OBD characteristic");
            sent = true;
        } else {
            Serial.println("  ✗ Failed to write to OBD characteristic");
        }
    }

    if (!sent) {
        Serial.println("ERROR: Could not send command - no writable characteristic");
    }

    // Wait for response
    Serial.println("Waiting for response...");
    delay(500);  // Give adapter time to respond
}

void runCommandSequence() {
    if (!connected || !servicesDiscovered) {
        return;
    }

    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════════════════════════╗");
    Serial.println("║          STARTING AT COMMAND SEQUENCE TEST                 ║");
    Serial.println("╔════════════════════════════════════════════════════════════╗");
    Serial.println();

    for (int i = 0; testCommands[i] != nullptr; i++) {
        Serial.printf("\n--- Test %d: %s ---\n", i + 1, testCommands[i]);
        sendCommand(testCommands[i]);
        delay(1000);  // Delay between commands
    }

    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════════════════════════╗");
    Serial.println("║          COMMAND SEQUENCE COMPLETE                         ║");
    Serial.println("╔════════════════════════════════════════════════════════════╗");
    Serial.println("\nDisconnecting from adapter...");

    pClient->disconnect();
}

void setup() {
    Serial.begin(115200);
    delay(2000);  // Wait for serial monitor

    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════════════════════════╗");
    Serial.println("║          MockStang BLE Sniffer - ESP32-S3                  ║");
    Serial.println("║          Real Vgate Adapter Communication Logger           ║");
    Serial.println("╔════════════════════════════════════════════════════════════╗");
    Serial.println();
    Serial.printf("Target device: %s\n", TARGET_DEVICE_NAME);
    Serial.println();

    Serial.println("Initializing NimBLE...");
    NimBLEDevice::init("MockStang-Sniffer");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    Serial.println("Starting BLE scan...");
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
    pScan->setActiveScan(true);
    pScan->setInterval(100);
    pScan->setWindow(99);
    pScan->start(0, false);  // Scan continuously until target found

    Serial.println("Scanning for IOS-Vlink adapter...");
}

void loop() {
    static bool commandsSent = false;

    if (doConnect) {
        doConnect = false;
        if (connectToServer()) {
            Serial.println("\nConnection successful!");
        } else {
            Serial.println("\nConnection failed, restarting scan...");
            NimBLEDevice::getScan()->start(0, false);
        }
    }

    if (connected && servicesDiscovered && !commandsSent) {
        delay(2000);  // Brief delay after connection
        runCommandSequence();
        commandsSent = true;
    }

    if (!connected && commandsSent) {
        // Reset for next connection
        commandsSent = false;
        delay(5000);
        Serial.println("Restarting scan...");
        NimBLEDevice::getScan()->start(0, false);
    }

    delay(1000);
}
