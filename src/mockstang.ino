#include <Arduino.h>
#include "config.h"

// Platform-specific WiFi includes
#ifdef ESP01_BUILD
    #include <ESP8266WiFi.h>
    #include <ESPAsyncTCP.h>
#elif defined(ESP32_BUILD)
    #include <WiFi.h>
    #include <AsyncTCP.h>
    #if ENABLE_BLE
        #include "ble_server.h"
    #endif
    #if ENABLE_DISPLAY
        #include "display_manager.h"
    #endif
#endif

#include "elm327_protocol.h"
#include "pid_handler.h"
#include "web_server.h"
#include "config_manager.h"

// ELM327 TCP Server
WiFiServer elm327Server(ELM327_PORT);
WiFiClient elm327Client;

// Protocol handlers
ELM327Protocol elm327;
PIDHandler* pidHandler;
WebServer* webServer;
ConfigManager* configManager;

#if ENABLE_BLE
    BLEServer* bleServer;
#endif

#if ENABLE_DISPLAY
    DisplayManager* display;
#endif

// Client connection state
bool clientConnected = false;
String inputBuffer = "";

// State broadcast timing
unsigned long lastStateBroadcast = 0;
#define STATE_BROADCAST_INTERVAL 200  // Broadcast state every 200ms during driving simulation

// Stats broadcast timing
unsigned long lastStatsBroadcast = 0;
#define STATS_BROADCAST_INTERVAL 1000  // Broadcast stats every 1 second

void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println("\n\n=================================");
    Serial.printf("MockStang - OBD-II Emulator (%s)\n", PLATFORM_NAME);
    Serial.println("=================================\n");

    // Load configuration from EEPROM
    configManager = new ConfigManager();
    configManager->load();

    // Initialize handlers
    pidHandler = new PIDHandler(&elm327, configManager);
    webServer = new WebServer(pidHandler, configManager);

    // Apply default PID values from config
    pidHandler->updateRPM(configManager->getDefaultRPM());
    pidHandler->updateSpeed(configManager->getDefaultSpeed());
    pidHandler->updateCoolantTemp(configManager->getDefaultCoolantTemp());
    pidHandler->updateIntakeTemp(configManager->getDefaultIntakeTemp());
    pidHandler->updateThrottle(configManager->getDefaultThrottle());
    pidHandler->updateMAF(configManager->getDefaultMAF());
    pidHandler->updateFuelLevel(configManager->getDefaultFuelLevel());
    pidHandler->updateBarometric(configManager->getDefaultBarometric());

    #if ENABLE_DISPLAY
        // Initialize TFT display (ESP32 only)
        display = new DisplayManager(pidHandler);
        display->begin();
        display->showSplash();
    #endif

    // Generate SSID (either custom or MAC-based)
    WiFi.mode(WIFI_AP);
    uint8_t mac[6];
    WiFi.softAPmacAddress(mac);
    char ssid[32];

    if (configManager->getUseCustomSSID() && strlen(configManager->getSSID()) > 0) {
        strncpy(ssid, configManager->getSSID(), 31);
        ssid[31] = 0;
    } else {
        sprintf(ssid, "%s%02X%02X", WIFI_SSID_PREFIX, mac[4], mac[5]);
    }

    // Get password from config
    const char* password = configManager->getUsePassword() ? configManager->getPassword() : "";

    // Configure WiFi Access Point with custom IP from config
    Serial.printf("Configuring Access Point: %s\n", ssid);
    WiFi.softAPConfig(configManager->getIP(), configManager->getGateway(), configManager->getSubnet());
    WiFi.softAP(ssid, password, WIFI_CHANNEL, false, MAX_CONNECTIONS);

    IPAddress IP = WiFi.softAPIP();
    Serial.printf("AP IP address: %s\n", IP.toString().c_str());
    if (strlen(password) > 0) {
        Serial.printf("AP Password: %s\n", password);
    } else {
        Serial.println("AP Password: (none - open network)");
    }
    Serial.printf("AP MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.printf("VIN: %s\n", configManager->getVIN());

    // Start ELM327 server
    elm327Server.begin();
    elm327Server.setNoDelay(true);
    Serial.printf("ELM327 server listening on port %d\n", ELM327_PORT);

    // Start web server
    webServer->begin();

    #if ENABLE_BLE
        // Initialize BLE server (ESP32 only)
        bleServer = new BLEServer(pidHandler, configManager);
        bleServer->begin();
        Serial.println("BLE server started and advertising");
    #endif

    Serial.println("\n=================================");
    Serial.println("System Ready!");
    Serial.printf("Connect to WiFi: %s\n", ssid);
    Serial.printf("OBD-II Port: %d\n", ELM327_PORT);
    Serial.printf("Web Dashboard: http://%s\n", IP.toString().c_str());
    Serial.println("=================================\n");
}

void loop() {
    // Handle web server
    webServer->loop();

    #if ENABLE_BLE
        // Handle BLE connections
        bleServer->loop();
    #endif

    #if ENABLE_DISPLAY
        // Update display
        display->update();
    #endif

    // Check for new ELM327 client connections
    if (elm327Server.hasClient()) {
        // If we already have a client, disconnect it
        if (clientConnected && elm327Client.connected()) {
            WiFiClient newClient = elm327Server.accept();
            Serial.println("New connection attempt - rejecting (already connected)");
            newClient.stop();
        } else {
            // Accept new client
            elm327Client = elm327Server.accept();
            if (elm327Client) {
                clientConnected = true;
                inputBuffer = "";
                Serial.printf("ELM327 client connected from %s\n",
                             elm327Client.remoteIP().toString().c_str());

                // Track connection statistics
                webServer->trackConnection(elm327Client.remoteIP().toString());

                // Send initial prompt
                elm327Client.print("ELM327 v1.5\r\r>");
            }
        }
    }

    // Handle existing client
    if (clientConnected && elm327Client.connected()) {
        // Read available data
        while (elm327Client.available()) {
            char c = elm327Client.read();

            // ELM327 protocol uses CR (0x0D) as command terminator
            if (c == '\r' || c == '\n') {
                if (inputBuffer.length() > 0) {
                    processCommand(inputBuffer);
                    inputBuffer = "";
                }
            } else if (c >= 32 && c < 127) {  // Printable characters only
                inputBuffer += c;
                if (inputBuffer.length() >= MAX_COMMAND_LENGTH) {
                    // Buffer overflow protection
                    inputBuffer = "";
                    elm327Client.print("BUFFER FULL\r\r>");
                }
            }
        }
    } else if (clientConnected) {
        // Client disconnected
        Serial.println("ELM327 client disconnected");
        elm327Client.stop();
        clientConnected = false;
        inputBuffer = "";

        // Track disconnection
        webServer->trackDisconnection();
    }

    // Handle WebSocket messages for parameter updates
    AsyncWebSocket* ws = webServer->getWebSocket();
    if (ws && ws->count() > 0) {
        // Check for WebSocket text frames
        // Note: Message handling is done in the WebSocket event callback
        // This section could be expanded to handle queued messages
    }

    // Update driving simulator
    pidHandler->updateDrivingSimulator();

    // Broadcast state updates during driving simulation
    if (pidHandler->getDriveMode() != DRIVE_OFF) {
        unsigned long now = millis();
        if (now - lastStateBroadcast >= STATE_BROADCAST_INTERVAL) {
            webServer->broadcastState(pidHandler->getState());
            lastStateBroadcast = now;
        }
    }

    // Broadcast connection statistics periodically
    unsigned long now = millis();
    if (now - lastStatsBroadcast >= STATS_BROADCAST_INTERVAL) {
        webServer->broadcastStats();
        lastStatsBroadcast = now;
    }

    delay(1);  // Small delay to prevent watchdog timer issues
}

void processCommand(String command) {
    command.trim();

    if (command.length() == 0) {
        return;
    }

    // Track command statistics
    webServer->trackCommand(command);

    #if ENABLE_SERIAL_LOGGING
        Serial.printf("CMD: %s\n", command.c_str());
    #endif

    String response;

    // Check if it's an AT command or OBD request
    if (command.startsWith("AT") || command.startsWith("at")) {
        response = elm327.handleCommand(command);
    } else {
        // OBD-II request - simulate ECU query delay
        // Real ELM327 adapters have 20-100ms delay for CAN bus communication
        delay(35);
        response = pidHandler->handleRequest(command);
    }

    // Send response to client
    elm327Client.print(response);

    // Track response
    webServer->trackResponse();

    #if ENABLE_SERIAL_LOGGING
        Serial.printf("RESP: %s\n", response.c_str());
    #endif

    // Broadcast to web interface
    webServer->broadcastOBDActivity(command, response);
}

// WebSocket message handler (called from async callback)
void onWsMessage(AsyncWebSocket *server, AsyncWebSocketClient *client,
                 AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            data[len] = 0;
            String message = (char*)data;
            webServer->handleWebSocketMessage(message);
        }
    }
}
