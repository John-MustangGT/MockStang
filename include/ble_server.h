#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#if ENABLE_BLE

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "config.h"
#include "pid_handler.h"
#include "config_manager.h"
#include "elm327_protocol.h"

/**
 * BLE Server Implementation for Vgate/Vlinker ELM327 Profile
 *
 * GATT Profile:
 * - Main Service UUID: E7810A71-73AE-499D-8C15-FAA9AEF0C3F2
 * - Characteristic UUID: BEF8D6C9-9C21-4C9E-B632-BD58C1009F9F (Read, Write, Notify)
 * - Device Information Service (DIS): 180A with manufacturer, model, etc.
 *
 * Operation:
 * - Advertises as "MockStang" BLE device
 * - Accepts ELM327 commands via Write
 * - Returns responses via Notify
 * - Supports multiple concurrent BLE clients
 * - Runs alongside WiFi server (dual-mode operation)
 */
class BLEOBDServer {
private:
    PIDHandler* pidHandler;
    ConfigManager* configManager;
    ELM327Protocol* elm327;

    NimBLEServer* pServer;
    NimBLECharacteristic* pOBDCharacteristic;

    bool deviceConnected;
    bool oldDeviceConnected;
    String inputBuffer;
    uint8_t connectedClients;

    // Server callbacks for connection management
    class ServerCallbacks: public NimBLEServerCallbacks {
        BLEOBDServer* parent;
    public:
        ServerCallbacks(BLEOBDServer* p) : parent(p) {}

        void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
            parent->deviceConnected = true;
            parent->connectedClients++;
            Serial.printf("BLE Client connected (total: %d)\n", parent->connectedClients);

            // Update connection parameters for lower latency
            pServer->updateConnParams(desc->conn_handle, 24, 48, 0, 60);

            // Small delay to let connection stabilize
            delay(100);

            // Send initial ELM327 greeting (required by OBD apps)
            if (parent->pOBDCharacteristic) {
                String greeting = "ELM327 v1.5\r\r>";
                parent->pOBDCharacteristic->setValue(greeting.c_str());
                parent->pOBDCharacteristic->notify();
                Serial.println("BLE: Sent greeting to client");
            }
        }

        void onDisconnect(NimBLEServer* pServer) {
            parent->deviceConnected = false;
            if (parent->connectedClients > 0) {
                parent->connectedClients--;
            }
            Serial.printf("BLE Client disconnected (remaining: %d)\n", parent->connectedClients);

            // Clear input buffer on disconnect
            parent->inputBuffer = "";
        }
    };

    // Characteristic callbacks for data handling
    class CharacteristicCallbacks: public NimBLECharacteristicCallbacks {
        BLEOBDServer* parent;
    public:
        CharacteristicCallbacks(BLEOBDServer* p) : parent(p) {}

        void onRead(NimBLECharacteristic* pCharacteristic) {
            // Some apps may read the characteristic
            #if ENABLE_SERIAL_LOGGING
                Serial.println("BLE characteristic read");
            #endif
        }

        void onWrite(NimBLECharacteristic* pCharacteristic) {
            std::string rxValue = pCharacteristic->getValue();
            if (rxValue.length() > 0) {
                // Handle incoming data
                for (char c : rxValue) {
                    // ELM327 uses \r as terminator
                    if (c == '\r' || c == '\n') {
                        if (parent->inputBuffer.length() > 0) {
                            parent->processCommand(parent->inputBuffer);
                            parent->inputBuffer = "";
                        }
                    } else if (c >= 32 && c < 127) {  // Printable characters
                        parent->inputBuffer += c;
                        if (parent->inputBuffer.length() >= MAX_COMMAND_LENGTH) {
                            parent->inputBuffer = "";  // Buffer overflow protection
                        }
                    }
                }
            }
        }
    };

public:
    BLEOBDServer(PIDHandler* handler, ConfigManager* config, ELM327Protocol* elm)
        : pidHandler(handler), configManager(config), elm327(elm),
          deviceConnected(false), oldDeviceConnected(false), connectedClients(0) {}

    void begin() {
        Serial.println("Initializing BLE (Vgate/Vlinker Profile)...");

        // Initialize NimBLE with minimal configuration
        NimBLEDevice::init(BLE_DEVICE_NAME);
        NimBLEDevice::setPower(ESP_PWR_LVL_P9); // Max power for better range
        NimBLEDevice::setMTU(512);  // Larger MTU for better throughput

        // Create BLE Server
        pServer = NimBLEDevice::createServer();
        pServer->setCallbacks(new ServerCallbacks(this));

        // ========================================
        // OBD-II Service (Vgate/Vlinker Profile)
        // ========================================
        NimBLEService* pOBDService = pServer->createService(BLE_SERVICE_UUID);

        // Single characteristic for Read, Write, Notify
        pOBDCharacteristic = pOBDService->createCharacteristic(
            BLE_CHAR_UUID,
            NIMBLE_PROPERTY::READ |
            NIMBLE_PROPERTY::WRITE |
            NIMBLE_PROPERTY::WRITE_NR |
            NIMBLE_PROPERTY::NOTIFY
        );
        pOBDCharacteristic->setCallbacks(new CharacteristicCallbacks(this));

        // Start OBD service
        pOBDService->start();

        // ========================================
        // Start Advertising (ONLY the OBD service)
        // ========================================
        NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();

        // Clear any default advertising data
        pAdvertising->reset();

        // Only advertise the OBD service UUID
        pAdvertising->addServiceUUID(BLE_SERVICE_UUID);

        // Set device name in advertising
        pAdvertising->setName(BLE_DEVICE_NAME);

        // Enable scan response to fit more data
        pAdvertising->setScanResponse(true);

        // Connection interval preferences (for iOS compatibility)
        pAdvertising->setMinPreferred(0x06);  // 7.5ms
        pAdvertising->setMaxPreferred(0x12);  // 22.5ms

        // Start advertising
        pAdvertising->start();

        Serial.printf("BLE server started:\n");
        Serial.printf("  Device Name: %s\n", BLE_DEVICE_NAME);
        Serial.printf("  OBD Service: %s\n", BLE_SERVICE_UUID);
        Serial.printf("  Characteristic: %s\n", BLE_CHAR_UUID);
        Serial.println("Ready for BLE connections...");
    }

    void loop() {
        // Handle disconnecting
        if (!deviceConnected && oldDeviceConnected) {
            delay(500); // Give the bluetooth stack time to get ready
            pServer->startAdvertising(); // Restart advertising
            Serial.println("BLE: Restarting advertising");
            oldDeviceConnected = deviceConnected;
        }

        // Handle connecting
        if (deviceConnected && !oldDeviceConnected) {
            oldDeviceConnected = deviceConnected;
        }
    }

    void processCommand(String command) {
        command.trim();

        if (command.length() == 0) {
            return;
        }

        #if ENABLE_SERIAL_LOGGING
            Serial.printf("BLE CMD: %s\n", command.c_str());
        #endif

        String response;

        // Process command through ELM327 protocol handler
        if (command.startsWith("AT") || command.startsWith("at")) {
            // AT command
            response = elm327->handleCommand(command);
        } else {
            // OBD-II request - simulate ECU query delay
            delay(35);
            response = pidHandler->handleRequest(command);
        }

        // Send response via BLE notification
        if (deviceConnected && pOBDCharacteristic) {
            // BLE can handle chunking automatically with MTU
            pOBDCharacteristic->setValue(response.c_str());
            pOBDCharacteristic->notify();

            #if ENABLE_SERIAL_LOGGING
                Serial.printf("BLE RESP: %s\n", response.c_str());
            #endif
        }
    }

    bool isConnected() {
        return deviceConnected;
    }

    uint8_t getConnectedClients() {
        return connectedClients;
    }
};

#endif // ENABLE_BLE

#endif // BLE_SERVER_H
