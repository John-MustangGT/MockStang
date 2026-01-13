#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#if ENABLE_BLE

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "config.h"
#include "pid_handler.h"
#include "config_manager.h"

class BLEServer {
private:
    PIDHandler* pidHandler;
    ConfigManager* configManager;

    NimBLEServer* pServer;
    NimBLECharacteristic* pTxCharacteristic;
    NimBLECharacteristic* pRxCharacteristic;

    bool deviceConnected;
    bool oldDeviceConnected;
    String inputBuffer;

    // Callbacks
    class ServerCallbacks: public NimBLEServerCallbacks {
        BLEServer* parent;
    public:
        ServerCallbacks(BLEServer* p) : parent(p) {}

        void onConnect(NimBLEServer* pServer) {
            parent->deviceConnected = true;
            Serial.println("BLE Client connected");
        }

        void onDisconnect(NimBLEServer* pServer) {
            parent->deviceConnected = false;
            Serial.println("BLE Client disconnected");
        }
    };

    class CharacteristicCallbacks: public NimBLECharacteristicCallbacks {
        BLEServer* parent;
    public:
        CharacteristicCallbacks(BLEServer* p) : parent(p) {}

        void onWrite(NimBLECharacteristic* pCharacteristic) {
            std::string rxValue = pCharacteristic->getValue();
            if (rxValue.length() > 0) {
                parent->processCommand(String(rxValue.c_str()));
            }
        }
    };

public:
    BLEServer(PIDHandler* handler, ConfigManager* config)
        : pidHandler(handler), configManager(config), deviceConnected(false), oldDeviceConnected(false) {}

    void begin() {
        Serial.println("Initializing BLE...");

        // Initialize NimBLE
        NimBLEDevice::init(BLE_DEVICE_NAME);
        NimBLEDevice::setPower(ESP_PWR_LVL_P9); // Max power

        // Create BLE Server
        pServer = NimBLEDevice::createServer();
        pServer->setCallbacks(new ServerCallbacks(this));

        // Create BLE Service
        NimBLEService* pService = pServer->createService(BLE_SERVICE_UUID);

        // Create TX Characteristic (notify)
        pTxCharacteristic = pService->createCharacteristic(
            BLE_CHAR_UUID_TX,
            NIMBLE_PROPERTY::NOTIFY
        );

        // Create RX Characteristic (write)
        pRxCharacteristic = pService->createCharacteristic(
            BLE_CHAR_UUID_RX,
            NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
        );
        pRxCharacteristic->setCallbacks(new CharacteristicCallbacks(this));

        // Start service
        pService->start();

        // Start advertising
        NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
        pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
        pAdvertising->setScanResponse(true);
        pAdvertising->setMinPreferred(0x06);  // Functions for better iOS compatibility
        pAdvertising->setMinPreferred(0x12);
        pAdvertising->start();

        Serial.println("BLE server started and advertising");
    }

    void loop() {
        // Handle disconnecting
        if (!deviceConnected && oldDeviceConnected) {
            delay(500); // Give the bluetooth stack time to get ready
            pServer->startAdvertising(); // Restart advertising
            Serial.println("Start advertising");
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

        // Check if it's an AT command or OBD request
        if (command.startsWith("AT") || command.startsWith("at")) {
            // Process AT command (would need access to elm327 protocol handler)
            // For now, simplified
            response = "OK\r\r>";
        } else {
            // OBD-II request
            delay(35); // Simulate ECU query delay
            response = pidHandler->handleRequest(command);
        }

        // Send response via BLE
        if (deviceConnected && pTxCharacteristic) {
            pTxCharacteristic->setValue(response.c_str());
            pTxCharacteristic->notify();
            #if ENABLE_SERIAL_LOGGING
                Serial.printf("BLE RESP: %s\n", response.c_str());
            #endif
        }
    }

    bool isConnected() {
        return deviceConnected;
    }
};

#endif // ENABLE_BLE

#endif // BLE_SERVER_H
