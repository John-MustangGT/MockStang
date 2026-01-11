#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include "web_interface.h"
#include "pid_handler.h"
#include "config_manager.h"

class WebServer {
private:
    AsyncWebServer* server;
    AsyncWebSocket* ws;
    PIDHandler* pidHandler;
    ConfigManager* configManager;
    static WebServer* instance;  // For static callback

    // WebSocket event handler
    static void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                                  AwsEventType type, void *arg, uint8_t *data, size_t len) {
        if (type == WS_EVT_CONNECT) {
            Serial.printf("WebSocket client #%u connected from %s\n",
                         client->id(), client->remoteIP().toString().c_str());
        }
        else if (type == WS_EVT_DISCONNECT) {
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
        }
        else if (type == WS_EVT_DATA) {
            // Handle incoming WebSocket data (parameter updates from web UI)
            AwsFrameInfo *info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                data[len] = 0;  // Null terminate
                String message = (char*)data;
                if (instance) {
                    instance->handleWebSocketMessage(message);
                }
            }
        }
    }

public:
    WebServer(PIDHandler* handler, ConfigManager* config) : pidHandler(handler), configManager(config) {
        instance = this;  // Set static instance for callback
        server = new AsyncWebServer(WEB_SERVER_PORT);
        ws = new AsyncWebSocket("/ws");
    }

    void begin() {
        // Attach WebSocket
        ws->onEvent(onWebSocketEvent);
        server->addHandler(ws);

        // Serve main page
        server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send_P(200, "text/html", INDEX_HTML);
        });

        // Serve settings page
        server->on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send_P(200, "text/html", SETTINGS_HTML);
        });

        // API: Get configuration
        server->on("/api/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
            String json = this->configManager->toJSON();
            request->send(200, "application/json", json);
        });

        // API: Save configuration
        server->on("/api/config", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
            [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
                // Parse JSON and update config
                String body = "";
                for (size_t i = 0; i < len; i++) {
                    body += (char)data[i];
                }

                // Simple JSON parsing
                if (body.indexOf("\"useCustomSSID\":true") >= 0) {
                    this->configManager->setUseCustomSSID(true);
                } else {
                    this->configManager->setUseCustomSSID(false);
                }

                int ssidStart = body.indexOf("\"ssid\":\"") + 8;
                int ssidEnd = body.indexOf("\"", ssidStart);
                if (ssidStart > 7 && ssidEnd > ssidStart) {
                    this->configManager->setSSID(body.substring(ssidStart, ssidEnd).c_str());
                }

                if (body.indexOf("\"usePassword\":true") >= 0) {
                    this->configManager->setUsePassword(true);
                } else {
                    this->configManager->setUsePassword(false);
                }

                int passStart = body.indexOf("\"password\":\"") + 12;
                int passEnd = body.indexOf("\"", passStart);
                if (passStart > 11 && passEnd > passStart) {
                    this->configManager->setPassword(body.substring(passStart, passEnd).c_str());
                }

                // Parse IP address
                int ipStart = body.indexOf("\"ip\":\"") + 6;
                int ipEnd = body.indexOf("\"", ipStart);
                if (ipStart > 5 && ipEnd > ipStart) {
                    String ipStr = body.substring(ipStart, ipEnd);
                    IPAddress ip;
                    if (ip.fromString(ipStr)) {
                        this->configManager->setIP(ip);
                    }
                }

                // Parse subnet
                int subnetStart = body.indexOf("\"subnet\":\"") + 10;
                int subnetEnd = body.indexOf("\"", subnetStart);
                if (subnetStart > 9 && subnetEnd > subnetStart) {
                    String subnetStr = body.substring(subnetStart, subnetEnd);
                    IPAddress subnet;
                    if (subnet.fromString(subnetStr)) {
                        this->configManager->setSubnet(subnet);
                    }
                }

                // Parse gateway
                int gatewayStart = body.indexOf("\"gateway\":\"") + 11;
                int gatewayEnd = body.indexOf("\"", gatewayStart);
                if (gatewayStart > 10 && gatewayEnd > gatewayStart) {
                    String gatewayStr = body.substring(gatewayStart, gatewayEnd);
                    IPAddress gateway;
                    if (gateway.fromString(gatewayStr)) {
                        this->configManager->setGateway(gateway);
                    }
                }

                // Parse VIN
                int vinStart = body.indexOf("\"vin\":\"") + 7;
                int vinEnd = body.indexOf("\"", vinStart);
                if (vinStart > 6 && vinEnd > vinStart) {
                    this->configManager->setVIN(body.substring(vinStart, vinEnd).c_str());
                }

                // Parse device ID
                int devStart = body.indexOf("\"deviceId\":\"") + 12;
                int devEnd = body.indexOf("\"", devStart);
                if (devStart > 11 && devEnd > devStart) {
                    this->configManager->setDeviceId(body.substring(devStart, devEnd).c_str());
                }

                // Parse default PID values
                int rpmIdx = body.indexOf("\"defaultRPM\":");
                if (rpmIdx >= 0) {
                    this->configManager->setDefaultRPM(body.substring(rpmIdx + 13).toInt());
                }

                int speedIdx = body.indexOf("\"defaultSpeed\":");
                if (speedIdx >= 0) {
                    this->configManager->setDefaultSpeed(body.substring(speedIdx + 15).toInt());
                }

                int coolantIdx = body.indexOf("\"defaultCoolantTemp\":");
                if (coolantIdx >= 0) {
                    this->configManager->setDefaultCoolantTemp(body.substring(coolantIdx + 21).toInt());
                }

                int intakeIdx = body.indexOf("\"defaultIntakeTemp\":");
                if (intakeIdx >= 0) {
                    this->configManager->setDefaultIntakeTemp(body.substring(intakeIdx + 20).toInt());
                }

                int throttleIdx = body.indexOf("\"defaultThrottle\":");
                if (throttleIdx >= 0) {
                    this->configManager->setDefaultThrottle(body.substring(throttleIdx + 18).toInt());
                }

                int mafIdx = body.indexOf("\"defaultMAF\":");
                if (mafIdx >= 0) {
                    this->configManager->setDefaultMAF(body.substring(mafIdx + 13).toInt());
                }

                int fuelIdx = body.indexOf("\"defaultFuelLevel\":");
                if (fuelIdx >= 0) {
                    this->configManager->setDefaultFuelLevel(body.substring(fuelIdx + 19).toInt());
                }

                int baroIdx = body.indexOf("\"defaultBarometric\":");
                if (baroIdx >= 0) {
                    this->configManager->setDefaultBarometric(body.substring(baroIdx + 20).toInt());
                }

                // Save to EEPROM
                this->configManager->save();

                request->send(200, "application/json", "{\"success\":true}");
            }
        );

        // API: Factory reset
        server->on("/api/reset", HTTP_POST, [this](AsyncWebServerRequest *request) {
            this->configManager->reset();
            request->send(200, "application/json", "{\"success\":true}");
        });

        // Start server
        server->begin();
        Serial.printf("Web server started on port %d\n", WEB_SERVER_PORT);
    }

    void loop() {
        // Clean up disconnected WebSocket clients
        ws->cleanupClients();
    }

    // Broadcast OBD-II command/response to all connected WebSocket clients
    void broadcastOBDActivity(String command, String response) {
        if (ws->count() > 0) {
            String json = "{\"cmd\":\"" + command + "\",\"resp\":\"" + jsonEscape(response) + "\"}";
            ws->textAll(json);
        }
    }

    // Handle WebSocket message (parameter update from web UI)
    void handleWebSocketMessage(String message) {
        // Simple JSON parsing (format: {"param":"rpm","value":1500})
        int paramStart = message.indexOf("\"param\":\"") + 9;
        int paramEnd = message.indexOf("\"", paramStart);
        String param = message.substring(paramStart, paramEnd);

        int valueStart = message.indexOf("\"value\":") + 8;
        int valueEnd = message.indexOf("}", valueStart);
        String valueStr = message.substring(valueStart, valueEnd);
        int value = valueStr.toInt();

        // Update PID handler
        if (param == "rpm") pidHandler->updateRPM(value);
        else if (param == "speed") pidHandler->updateSpeed(value);
        else if (param == "coolant") pidHandler->updateCoolantTemp(value);
        else if (param == "intake") pidHandler->updateIntakeTemp(value);
        else if (param == "throttle") pidHandler->updateThrottle(value);
        else if (param == "maf") pidHandler->updateMAF(value);
        else if (param == "fuel") pidHandler->updateFuelLevel(value);
        else if (param == "baro") pidHandler->updateBarometric(value);

        // Handle commands
        if (message.indexOf("\"cmd\":\"reset_runtime\"") >= 0) {
            pidHandler->resetRuntime();
        }

        Serial.printf("Parameter updated: %s = %d\n", param.c_str(), value);
    }

    // Check for WebSocket messages
    void checkWebSocketMessages() {
        // Note: In a real implementation, we'd need to store the message
        // This is handled via the onWebSocketEvent callback
        // For simplicity, we'll handle parameter updates in the main loop
    }

    AsyncWebSocket* getWebSocket() { return ws; }

private:
    // Escape special characters for JSON
    String jsonEscape(String str) {
        str.replace("\\", "\\\\");
        str.replace("\"", "\\\"");
        str.replace("\r", "\\r");
        str.replace("\n", "\\n");
        return str;
    }
};

// Static instance definition
WebServer* WebServer::instance = nullptr;

#endif // WEB_SERVER_H
