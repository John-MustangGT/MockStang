#include <ESP8266WiFi.h>
#include "config.h"
#include "elm327_protocol.h"
#include "pid_handler.h"
#include "web_server.h"

// ELM327 TCP Server
WiFiServer elm327Server(ELM327_PORT);
WiFiClient elm327Client;

// Protocol handlers
ELM327Protocol elm327;
PIDHandler* pidHandler;
WebServer* webServer;

// Client connection state
bool clientConnected = false;
String inputBuffer = "";

void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println("\n\n=================================");
    Serial.println("MockStang - WiFi OBD-II Emulator");
    Serial.println("=================================\n");

    // Initialize handlers
    pidHandler = new PIDHandler(&elm327);
    webServer = new WebServer(pidHandler);

    // Generate SSID from MAC address (iCAR_PRO_XXXX format)
    WiFi.mode(WIFI_AP);
    uint8_t mac[6];
    WiFi.softAPmacAddress(mac);
    char ssid[20];
    sprintf(ssid, "%s%02X%02X", WIFI_SSID_PREFIX, mac[4], mac[5]);

    // Configure WiFi Access Point with custom IP
    Serial.printf("Configuring Access Point: %s\n", ssid);
    WiFi.softAPConfig(AP_IP_ADDRESS, AP_GATEWAY, AP_SUBNET);
    WiFi.softAP(ssid, WIFI_PASSWORD, WIFI_CHANNEL, false, MAX_CONNECTIONS);

    IPAddress IP = WiFi.softAPIP();
    Serial.printf("AP IP address: %s\n", IP.toString().c_str());
    if (strlen(WIFI_PASSWORD) > 0) {
        Serial.printf("AP Password: %s\n", WIFI_PASSWORD);
    } else {
        Serial.println("AP Password: (none - open network)");
    }
    Serial.printf("AP MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Start ELM327 server
    elm327Server.begin();
    elm327Server.setNoDelay(true);
    Serial.printf("ELM327 server listening on port %d\n", ELM327_PORT);

    // Start web server
    webServer->begin();

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

    // Check for new ELM327 client connections
    if (elm327Server.hasClient()) {
        // If we already have a client, disconnect it
        if (clientConnected && elm327Client.connected()) {
            WiFiClient newClient = elm327Server.available();
            Serial.println("New connection attempt - rejecting (already connected)");
            newClient.stop();
        } else {
            // Accept new client
            elm327Client = elm327Server.available();
            if (elm327Client) {
                clientConnected = true;
                inputBuffer = "";
                Serial.printf("ELM327 client connected from %s\n",
                             elm327Client.remoteIP().toString().c_str());

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
    }

    // Handle WebSocket messages for parameter updates
    AsyncWebSocket* ws = webServer->getWebSocket();
    if (ws && ws->count() > 0) {
        // Check for WebSocket text frames
        // Note: Message handling is done in the WebSocket event callback
        // This section could be expanded to handle queued messages
    }

    delay(1);  // Small delay to prevent watchdog timer issues
}

void processCommand(String command) {
    command.trim();

    if (command.length() == 0) {
        return;
    }

    Serial.printf("CMD: %s\n", command.c_str());

    String response;

    // Check if it's an AT command or OBD request
    if (command.startsWith("AT") || command.startsWith("at")) {
        response = elm327.handleCommand(command);
    } else {
        // OBD-II request
        response = pidHandler->handleRequest(command);
    }

    // Send response to client
    elm327Client.print(response);

    Serial.printf("RESP: %s", response.c_str());

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
