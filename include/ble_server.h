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
    NimBLECharacteristic* pCustomNotifyCharacteristic;  // 0x2AF0 for Custom Service responses

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

            // Let client negotiate connection parameters
            // Don't force parameters immediately - can cause disconnects
        }

        void onMTUChange(uint16_t MTU, ble_gap_conn_desc* desc) {
            Serial.printf("BLE: MTU changed to %d\n", MTU);
            Serial.println("BLE: Waiting for client to subscribe to notifications...");
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
            Serial.println("BLE: Client read OBD characteristic");
            Serial.printf("  UUID: %s\n", pCharacteristic->getUUID().toString().c_str());
            Serial.printf("  Current value: %s\n", pCharacteristic->getValue().c_str());
        }

        void onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue) {
            // Client has subscribed/unsubscribed to notifications
            if (subValue > 0) {
                Serial.println("BLE: Client subscribed to OBD notifications");
                // Don't send unsolicited greeting - wait for ATZ command
            } else {
                Serial.println("BLE: Client unsubscribed from OBD notifications");
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
          deviceConnected(false), oldDeviceConnected(false), connectedClients(0),
          pOBDCharacteristic(nullptr), pCustomNotifyCharacteristic(nullptr) {}

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

        // Callbacks for DIS characteristics to see what OBD Doctor reads
        class DISCallbacks: public NimBLECharacteristicCallbacks {
            void onRead(NimBLECharacteristic* pCharacteristic) {
                Serial.printf("BLE: DIS Read - UUID: %s\n", pCharacteristic->getUUID().toString().c_str());
            }
        };

        // System ID (2A23) [Read]
        NimBLECharacteristic* pSystemID = pDISService->createCharacteristic(
            NimBLEUUID((uint16_t)0x2A23), NIMBLE_PROPERTY::READ
        );
        uint8_t systemID[8] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
        pSystemID->setValue(systemID, 8);
        pSystemID->setCallbacks(new DISCallbacks());

        // Manufacturer Name (2A29) [Read]
        NimBLECharacteristic* pManufacturer = pDISService->createCharacteristic(
            NimBLEUUID((uint16_t)0x2A29), NIMBLE_PROPERTY::READ
        );
        pManufacturer->setValue("Vgate");
        pManufacturer->setCallbacks(new DISCallbacks());

        pDISService->start();

        // ========================================
        // Custom Service 18F0 - Real Vgate has this (unknown purpose)
        // ========================================
        NimBLEService* pCustomService = pServer->createService(NimBLEUUID((uint16_t)0x18F0));

        // Callbacks for custom service - process commands written to 0x2AF1
        class CustomCallbacks: public NimBLECharacteristicCallbacks {
            BLEOBDServer* parent;
        public:
            CustomCallbacks(BLEOBDServer* p) : parent(p) {}

            void onRead(NimBLECharacteristic* pCharacteristic) {
                Serial.printf("BLE: Custom Service Read - UUID: %s\n", pCharacteristic->getUUID().toString().c_str());
            }

            void onWrite(NimBLECharacteristic* pCharacteristic) {
                std::string rxValue = pCharacteristic->getValue();
                Serial.printf("BLE: Custom Service Write - UUID: %s, Data (%d bytes): ",
                             pCharacteristic->getUUID().toString().c_str(), rxValue.length());
                for (uint8_t b : rxValue) {
                    Serial.printf("0x%02X ", b);
                }
                Serial.println();

                // Process commands from Custom Service the same way as OBD Service
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
                    }
                }
            }
        };

        // Characteristic 2AF1 [Write, Write_NR] - Commands come here for some OBD apps
        // Real Vgate has BOTH Write and Write_NR properties
        NimBLECharacteristic* pCustomWrite = pCustomService->createCharacteristic(
            NimBLEUUID((uint16_t)0x2AF1),
            NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
        );
        pCustomWrite->setCallbacks(new CustomCallbacks(this));

        // Callbacks for Custom Notify characteristic to track subscription/reads
        class CustomNotifyCallbacks: public NimBLECharacteristicCallbacks {
            void onRead(NimBLECharacteristic* pCharacteristic) {
                Serial.println("BLE: Custom Service 0x2AF0 - Client READ response");
            }
            void onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue) {
                if (subValue > 0) {
                    Serial.println("BLE: Custom Service 0x2AF0 - Client SUBSCRIBED to notifications");
                } else {
                    Serial.println("BLE: Custom Service 0x2AF0 - Client UNSUBSCRIBED from notifications");
                }
            }
        };

        // Characteristic 2AF0 [Notify, Indicate] - Responses go here for some OBD apps
        // Real Vgate has Notify + Indicate (NO Read property!)
        pCustomNotifyCharacteristic = pCustomService->createCharacteristic(
            NimBLEUUID((uint16_t)0x2AF0),
            NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::INDICATE
        );
        pCustomNotifyCharacteristic->setCallbacks(new CustomNotifyCallbacks());

        pCustomService->start();

        // ========================================
        // OBD-II Service (E7810A71-...) - The main service
        // ========================================
        NimBLEService* pOBDService = pServer->createService(BLE_SERVICE_UUID);

        // Match real Vgate exactly: Read, Write, Notify, Indicate
        pOBDCharacteristic = pOBDService->createCharacteristic(
            BLE_CHAR_UUID,
            NIMBLE_PROPERTY::READ |
            NIMBLE_PROPERTY::WRITE |
            NIMBLE_PROPERTY::NOTIFY |
            NIMBLE_PROPERTY::INDICATE
        );
        pOBDCharacteristic->setCallbacks(new CharacteristicCallbacks(this));

        // Set initial greeting value so it can be read immediately
        pOBDCharacteristic->setValue("ELM327 v1.5\r\r>");

        pOBDService->start();

        // ========================================
        // Start Advertising - Match real Vgate adapter exactly
        // ========================================
        NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();

        // Real Vgate advertises ONLY 0x18F0 custom service
        // All other services are discovered after connection
        pAdvertising->addServiceUUID(NimBLEUUID((uint16_t)0x18F0)); // Custom service

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

        // Process command through ELM327 protocol handler
        // This returns the FULL response including echo (if enabled)
        String fullResponse;
        if (command.startsWith("AT") || command.startsWith("at")) {
            // AT command
            fullResponse = elm327->handleCommand(command);
        } else {
            // OBD-II request - simulate ECU query delay
            delay(35);
            fullResponse = pidHandler->handleRequest(command);
        }

        // Real Vgate adapter sends echo and response as SEPARATE notifications!
        // Parse the response to split echo from actual response
        #if ENABLE_SERIAL_LOGGING
            Serial.printf("BLE: Echo enabled: %s\n", elm327->isEchoEnabled() ? "YES" : "NO");
        #endif

        if (deviceConnected && elm327->isEchoEnabled()) {
            // Echo is enabled - need to split into two notifications
            // Format: "CMD\rRESPONSE\r\r>"
            int firstCR = fullResponse.indexOf('\r');

            #if ENABLE_SERIAL_LOGGING
                Serial.printf("BLE: First CR position: %d\n", firstCR);
            #endif

            if (firstCR > 0) {
                String echo = fullResponse.substring(0, firstCR + 1);  // "CMD\r"
                String response = fullResponse.substring(firstCR + 1);  // "\rRESPONSE\r\r>"

                #if ENABLE_SERIAL_LOGGING
                    Serial.printf("BLE: Sending ECHO (%d bytes): ", echo.length());
                    for (char c : echo) {
                        if (c >= 32 && c < 127) Serial.print(c);
                        else Serial.printf("[0x%02X]", (uint8_t)c);
                    }
                    Serial.println();
                #endif

                // Send echo first
                sendBLEResponse(echo);

                // Brief delay between echo and response (like real adapter)
                delay(10);

                #if ENABLE_SERIAL_LOGGING
                    Serial.printf("BLE: Sending RESPONSE (%d bytes): ", response.length());
                    for (char c : response) {
                        if (c >= 32 && c < 127) Serial.print(c);
                        else Serial.printf("[0x%02X]", (uint8_t)c);
                    }
                    Serial.println();
                #endif

                // Send actual response
                sendBLEResponse(response);
            } else {
                #if ENABLE_SERIAL_LOGGING
                    Serial.println("BLE: No CR found, sending as single");
                #endif
                sendBLEResponse(fullResponse);
            }
        } else {
            #if ENABLE_SERIAL_LOGGING
                Serial.println("BLE: Echo disabled, sending as single");
            #endif
            sendBLEResponse(fullResponse);
        }

        #if ENABLE_SERIAL_LOGGING
            Serial.printf("BLE RESP (%d bytes): ", fullResponse.length());
            for (char c : fullResponse) {
                if (c >= 32 && c < 127) Serial.print(c);
                else Serial.printf("[0x%02X]", (uint8_t)c);
            }
            Serial.println();
        #endif
    }

    void sendBLEResponse(const String& response) {
        if (!deviceConnected) return;

        // Send to main OBD characteristic (supports both Notify and Indicate)
        if (pOBDCharacteristic) {
            pOBDCharacteristic->setValue(response.c_str());
            pOBDCharacteristic->notify();    // Fire-and-forget
            pOBDCharacteristic->indicate();  // Requires acknowledgment
        }

        // Also send to Custom Service notify characteristic (0x2AF0)
        // Real Vgate has both Notify and Indicate on this characteristic
        if (pCustomNotifyCharacteristic) {
            pCustomNotifyCharacteristic->setValue(response.c_str());
            pCustomNotifyCharacteristic->notify();    // Fire-and-forget
            pCustomNotifyCharacteristic->indicate();  // Requires acknowledgment
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
