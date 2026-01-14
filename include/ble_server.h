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
            Serial.printf("  Connection handle: %d\n", desc->conn_handle);
            Serial.printf("  MTU: %d\n", pServer->getPeerMTU(desc->conn_handle));

            // Update connection parameters for lower latency
            int rc = pServer->updateConnParams(desc->conn_handle, 24, 48, 0, 60);
            Serial.printf("  Connection params update: %s\n", rc == 0 ? "OK" : "FAILED");

            Serial.println("BLE: Waiting for client to subscribe to notifications...");
        }

        void onMTUChange(uint16_t MTU, ble_gap_conn_desc* desc) {
            Serial.printf("BLE: MTU changed to %d\n", MTU);
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

        uint32_t onPassKeyRequest() {
            Serial.println("BLE: Passkey request (returning 0)");
            return 0;
        }

        void onAuthenticationComplete(ble_gap_conn_desc* desc) {
            Serial.printf("BLE: Authentication complete, status: %d\n", desc->sec_state.encrypted);
        }
    };

    // Characteristic callbacks for data handling
    class CharacteristicCallbacks: public NimBLECharacteristicCallbacks {
        BLEOBDServer* parent;
    public:
        CharacteristicCallbacks(BLEOBDServer* p) : parent(p) {}

        void onRead(NimBLECharacteristic* pCharacteristic) {
            // Some apps may read the characteristic to get the greeting
            Serial.print("BLE: Client read characteristic, value: ");
            Serial.println(pCharacteristic->getValue().c_str());
        }

        void onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue) {
            // Client has subscribed to notifications - send ELM327 greeting
            if (subValue > 0) {
                Serial.println("BLE: Client subscribed to notifications");
                delay(100);  // Brief delay for stability

                String greeting = "ELM327 v1.5\r\r>";
                pCharacteristic->setValue(greeting.c_str());
                pCharacteristic->notify();
                Serial.println("BLE: Sent ELM327 greeting");
            } else {
                Serial.println("BLE: Client unsubscribed from notifications");
            }
        }

        void onWrite(NimBLECharacteristic* pCharacteristic) {
            std::string rxValue = pCharacteristic->getValue();
            if (rxValue.length() > 0) {
                #if ENABLE_SERIAL_LOGGING
                    Serial.printf("BLE RX (%d bytes): ", rxValue.length());
                    for (char c : rxValue) {
                        if (c >= 32 && c < 127) Serial.print(c);
                        else Serial.printf("[0x%02X]", (uint8_t)c);
                    }
                    Serial.println();
                #endif

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
                            Serial.println("BLE: Buffer overflow - clearing");
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

        // Initialize NimBLE
        NimBLEDevice::init(BLE_DEVICE_NAME);
        NimBLEDevice::setPower(ESP_PWR_LVL_P9);
        NimBLEDevice::setMTU(512);

        // Create BLE Server
        pServer = NimBLEDevice::createServer();
        pServer->setCallbacks(new ServerCallbacks(this));

        // ========================================
        // Battery Service (180F) - Real Vgate has this
        // ========================================
        NimBLEService* pBatteryService = pServer->createService(NimBLEUUID((uint16_t)0x180F));
        NimBLECharacteristic* pBatteryLevel = pBatteryService->createCharacteristic(
            NimBLEUUID((uint16_t)0x2A19),
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
        );
        uint8_t batteryLevel = 100;
        pBatteryLevel->setValue(&batteryLevel, 1);
        pBatteryService->start();

        // ========================================
        // Link Loss Service (1803) - Real Vgate has this
        // ========================================
        NimBLEService* pLinkLossService = pServer->createService(NimBLEUUID((uint16_t)0x1803));
        NimBLECharacteristic* pLinkLossAlert = pLinkLossService->createCharacteristic(
            NimBLEUUID((uint16_t)0x2A06),
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
        );
        uint8_t alertLevel = 0;
        pLinkLossAlert->setValue(&alertLevel, 1);
        pLinkLossService->start();

        // ========================================
        // Immediate Alert Service (1802) - Real Vgate has this
        // ========================================
        NimBLEService* pImmediateAlertService = pServer->createService(NimBLEUUID((uint16_t)0x1802));
        NimBLECharacteristic* pImmediateAlert = pImmediateAlertService->createCharacteristic(
            NimBLEUUID((uint16_t)0x2A06),
            NIMBLE_PROPERTY::WRITE_NR
        );
        pImmediateAlertService->start();

        // ========================================
        // Alert Notification Service (1811) - Real Vgate has this
        // ========================================
        NimBLEService* pAlertNotifyService = pServer->createService(NimBLEUUID((uint16_t)0x1811));

        // Supported New Alert Category (2A47) [Read]
        NimBLECharacteristic* pSupportedNew = pAlertNotifyService->createCharacteristic(
            NimBLEUUID((uint16_t)0x2A47), NIMBLE_PROPERTY::READ
        );

        // New Alert (2A46) [Notify]
        NimBLECharacteristic* pNewAlert = pAlertNotifyService->createCharacteristic(
            NimBLEUUID((uint16_t)0x2A46), NIMBLE_PROPERTY::NOTIFY
        );

        // Supported Unread Alert Category (2A48) [Read]
        NimBLECharacteristic* pSupportedUnread = pAlertNotifyService->createCharacteristic(
            NimBLEUUID((uint16_t)0x2A48), NIMBLE_PROPERTY::READ
        );

        // Unread Alert Status (2A45) [Notify]
        NimBLECharacteristic* pUnreadAlert = pAlertNotifyService->createCharacteristic(
            NimBLEUUID((uint16_t)0x2A45), NIMBLE_PROPERTY::NOTIFY
        );

        // Alert Notification Control Point (2A44) [Write, Notify]
        NimBLECharacteristic* pAlertControl = pAlertNotifyService->createCharacteristic(
            NimBLEUUID((uint16_t)0x2A44),
            NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY
        );

        pAlertNotifyService->start();

        // ========================================
        // Tx Power Service (1804) - Real Vgate has this
        // ========================================
        NimBLEService* pTxPowerService = pServer->createService(NimBLEUUID((uint16_t)0x1804));
        NimBLECharacteristic* pTxPowerLevel = pTxPowerService->createCharacteristic(
            NimBLEUUID((uint16_t)0x2A07), NIMBLE_PROPERTY::READ
        );
        int8_t txPower = 0;
        pTxPowerLevel->setValue((uint8_t*)&txPower, 1);
        pTxPowerService->start();

        // ========================================
        // Device Information Service (180A) - Real Vgate has this
        // Only 2A23 (System ID) and 2A29 (Manufacturer) like real device
        // ========================================
        NimBLEService* pDISService = pServer->createService(NimBLEUUID((uint16_t)0x180A));

        // System ID (2A23) [Read]
        NimBLECharacteristic* pSystemID = pDISService->createCharacteristic(
            NimBLEUUID((uint16_t)0x2A23), NIMBLE_PROPERTY::READ
        );
        uint8_t systemID[8] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
        pSystemID->setValue(systemID, 8);

        // Manufacturer Name (2A29) [Read]
        NimBLECharacteristic* pManufacturer = pDISService->createCharacteristic(
            NimBLEUUID((uint16_t)0x2A29), NIMBLE_PROPERTY::READ
        );
        pManufacturer->setValue("Vgate");

        pDISService->start();

        // ========================================
        // Custom Service 18F0 - Real Vgate has this (unknown purpose)
        // ========================================
        NimBLEService* pCustomService = pServer->createService(NimBLEUUID((uint16_t)0x18F0));

        // Characteristic 2AF1 [Write]
        NimBLECharacteristic* pCustomWrite = pCustomService->createCharacteristic(
            NimBLEUUID((uint16_t)0x2AF1), NIMBLE_PROPERTY::WRITE
        );

        // Characteristic 2AF0 [Notify]
        NimBLECharacteristic* pCustomNotify = pCustomService->createCharacteristic(
            NimBLEUUID((uint16_t)0x2AF0), NIMBLE_PROPERTY::NOTIFY
        );

        pCustomService->start();

        // ========================================
        // OBD-II Service (E7810A71-...) - The main service
        // ========================================
        NimBLEService* pOBDService = pServer->createService(BLE_SERVICE_UUID);

        // Match real Vgate exactly: Read, Write, Notify (no Write_NR)
        pOBDCharacteristic = pOBDService->createCharacteristic(
            BLE_CHAR_UUID,
            NIMBLE_PROPERTY::READ |
            NIMBLE_PROPERTY::WRITE |
            NIMBLE_PROPERTY::NOTIFY
        );
        pOBDCharacteristic->setCallbacks(new CharacteristicCallbacks(this));

        // Set initial greeting value so it can be read immediately
        pOBDCharacteristic->setValue("ELM327 v1.5\r\r>");

        pOBDService->start();

        // ========================================
        // Start Advertising - Advertise ALL services like real device
        // ========================================
        NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();

        // Advertise all services (match real Vgate device)
        pAdvertising->addServiceUUID(NimBLEUUID((uint16_t)0x180F)); // Battery
        pAdvertising->addServiceUUID(NimBLEUUID((uint16_t)0x1803)); // Link Loss
        pAdvertising->addServiceUUID(NimBLEUUID((uint16_t)0x1802)); // Immediate Alert
        pAdvertising->addServiceUUID(NimBLEUUID((uint16_t)0x1811)); // Alert Notification
        pAdvertising->addServiceUUID(NimBLEUUID((uint16_t)0x1804)); // Tx Power
        pAdvertising->addServiceUUID(NimBLEUUID((uint16_t)0x18F0)); // Custom service
        pAdvertising->addServiceUUID(NimBLEUUID((uint16_t)0x180A)); // Device Info
        pAdvertising->addServiceUUID(BLE_SERVICE_UUID); // OBD Service

        pAdvertising->setName(BLE_DEVICE_NAME);
        pAdvertising->setScanResponse(true);
        pAdvertising->setMinPreferred(0x06);
        pAdvertising->setMaxPreferred(0x12);

        pAdvertising->start();

        Serial.println("BLE server started with full Vgate/Vlinker profile:");
        Serial.printf("  Device Name: %s\n", BLE_DEVICE_NAME);
        Serial.printf("  Services: Battery, Link Loss, Alerts, Tx Power, DIS, OBD\n");
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
