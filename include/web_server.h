#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include "web_interface.h"
#include "pid_handler.h"

class WebServer {
private:
    AsyncWebServer* server;
    AsyncWebSocket* ws;
    PIDHandler* pidHandler;
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
    WebServer(PIDHandler* handler) : pidHandler(handler) {
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
