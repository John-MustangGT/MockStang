#ifndef PID_HANDLER_H
#define PID_HANDLER_H

#include <Arduino.h>
#include "config.h"
#include "elm327_protocol.h"

// Forward declaration
class ConfigManager;

class PIDHandler {
private:
    CarState currentState;
    ELM327Protocol* elm;
    ConfigManager* config;
    unsigned long startTime;

public:
    PIDHandler(ELM327Protocol* elmProtocol, ConfigManager* configMgr) : elm(elmProtocol), config(configMgr) {
        currentState = DEFAULT_CAR_STATE;
        startTime = millis();
    }

    // Update a specific car parameter
    void updateRPM(uint16_t rpm) { currentState.rpm = rpm; }
    void updateSpeed(uint8_t speed) { currentState.speed = speed; }
    void updateCoolantTemp(uint8_t temp) { currentState.coolant_temp = temp; }
    void updateIntakeTemp(uint8_t temp) { currentState.intake_temp = temp; }
    void updateThrottle(uint8_t throttle) { currentState.throttle = throttle; }
    void updateMAF(uint16_t maf) { currentState.maf = maf; }
    void updateFuelLevel(uint8_t level) { currentState.fuel_level = level; }
    void updateBarometric(uint8_t baro) { currentState.barometric = baro; }
    void updateMILDistance(uint16_t dist) { currentState.mil_distance = dist; }

    // MIL and DTC management
    void setMIL(bool on) { currentState.mil_on = on; }
    bool getMIL() { return currentState.mil_on; }

    // Add a DTC (P0xxx, P2xxx, etc.)
    bool addDTC(uint16_t dtc) {
        if (currentState.dtc_count >= MAX_DTCS) return false;
        // Check if DTC already exists
        for (uint8_t i = 0; i < currentState.dtc_count; i++) {
            if (currentState.dtcs[i] == dtc) return false;
        }
        currentState.dtcs[currentState.dtc_count++] = dtc;
        currentState.mil_on = true; // Turn on MIL when DTC added
        return true;
    }

    // Remove a specific DTC
    bool removeDTC(uint16_t dtc) {
        for (uint8_t i = 0; i < currentState.dtc_count; i++) {
            if (currentState.dtcs[i] == dtc) {
                // Shift remaining DTCs down
                for (uint8_t j = i; j < currentState.dtc_count - 1; j++) {
                    currentState.dtcs[j] = currentState.dtcs[j + 1];
                }
                currentState.dtc_count--;
                if (currentState.dtc_count == 0) {
                    currentState.mil_on = false; // Turn off MIL when no DTCs
                }
                return true;
            }
        }
        return false;
    }

    // Clear all DTCs
    void clearDTCs() {
        currentState.dtc_count = 0;
        currentState.mil_on = false;
        currentState.mil_distance = 0;
        for (uint8_t i = 0; i < MAX_DTCS; i++) {
            currentState.dtcs[i] = 0;
        }
    }

    uint8_t getDTCCount() { return currentState.dtc_count; }
    uint16_t getDTC(uint8_t index) {
        if (index < currentState.dtc_count) {
            return currentState.dtcs[index];
        }
        return 0;
    }

    // Get current car state (for web interface)
    CarState getState() { return currentState; }

    // Update runtime counter
    void updateRuntime() {
        currentState.runtime = (millis() - startTime) / 1000;
    }

    // Reset runtime counter
    void resetRuntime() {
        startTime = millis();
        currentState.runtime = 0;
    }

    // Handle OBD-II mode 01 request
    String handleMode01(uint8_t pid) {
        uint8_t data[4];
        uint8_t dataLen = 0;

        switch (pid) {
            case 0x00:  // PIDs supported [01-20]
                // Bitmap of supported PIDs
                // We support: 0x01, 0x03, 0x04, 0x05, 0x0C, 0x0D, 0x0F, 0x10, 0x11, 0x1F
                data[0] = 0b10111000;  // 0x01, 0x03, 0x04, 0x05
                data[1] = 0b00011011;  // 0x0C, 0x0D, 0x0F, 0x10
                data[2] = 0b10000000;  // 0x11
                data[3] = 0b00000011;  // 0x1F, 0x20 (supports next range)
                dataLen = 4;
                break;

            case 0x01: {  // Monitor status since DTCs cleared
                // Byte A: MIL status and DTC count
                // Bit 7: MIL status (0=off, 1=on)
                // Bits 6-0: Number of DTCs (0-127)
                uint8_t dtcCount = currentState.dtc_count > 127 ? 127 : currentState.dtc_count;
                data[0] = (currentState.mil_on ? 0x80 : 0x00) | dtcCount;
                data[1] = 0x07;  // Test available flags
                data[2] = 0x65;  // Test complete flags
                data[3] = 0x04;  // Additional flags
                dataLen = 4;
                break;
            }

            case 0x03:  // Fuel system status
                // Closed loop, using O2 sensor
                data[0] = 0x02;  // System 1: Closed loop
                data[1] = 0x00;  // System 2: Not available
                dataLen = 2;
                break;

            case 0x04:  // Calculated engine load
                // Return 20% load (arbitrary)
                data[0] = (20 * 255) / 100;
                dataLen = 1;
                break;

            case 0x05:  // Engine coolant temperature
                data[0] = currentState.coolant_temp + 40;  // °C + 40
                dataLen = 1;
                break;

            case 0x0C: {  // Engine RPM
                // RPM is encoded as (RPM * 4) as 16-bit big-endian
                // Formula: RPM = ((A * 256) + B) / 4
                uint16_t encodedRPM = currentState.rpm * 4;
                data[0] = encodedRPM >> 8;
                data[1] = encodedRPM & 0xFF;
                dataLen = 2;
                break;
            }

            case 0x0D:  // Vehicle speed
                data[0] = currentState.speed;
                dataLen = 1;
                break;

            case 0x0F:  // Intake air temperature
                data[0] = currentState.intake_temp + 40;  // °C + 40
                dataLen = 1;
                break;

            case 0x10:  // MAF air flow rate
                data[0] = currentState.maf >> 8;
                data[1] = currentState.maf & 0xFF;
                dataLen = 2;
                break;

            case 0x11:  // Throttle position
                data[0] = (currentState.throttle * 255) / 100;
                dataLen = 1;
                break;

            case 0x1F:  // Run time since engine start
                updateRuntime();
                data[0] = currentState.runtime >> 8;
                data[1] = currentState.runtime & 0xFF;
                dataLen = 2;
                break;

            case 0x20:  // PIDs supported [21-40]
                data[0] = 0b10000000;  // 0x21 supported
                data[1] = 0b00000010;  // 0x2F supported
                data[2] = 0b00100000;  // 0x33 supported
                data[3] = 0b00000001;  // 0x40 supported
                dataLen = 4;
                break;

            case 0x21:  // Distance traveled with MIL on
                data[0] = currentState.mil_distance >> 8;
                data[1] = currentState.mil_distance & 0xFF;
                dataLen = 2;
                break;

            case 0x2F:  // Fuel tank level input
                data[0] = (currentState.fuel_level * 255) / 100;
                dataLen = 1;
                break;

            case 0x33:  // Barometric pressure
                data[0] = currentState.barometric;
                dataLen = 1;
                break;

            case 0x40:  // PIDs supported [41-60]
                // No additional PIDs supported
                data[0] = 0x00;
                data[1] = 0x00;
                data[2] = 0x00;
                data[3] = 0x00;
                dataLen = 4;
                break;

            default:
                // Unsupported PID
                return "NO DATA\r\r>";
        }

        return elm->formatOBDResponse(0x01, pid, data, dataLen);
    }

    // Handle OBD request string (e.g., "01 0C" for RPM)
    // Handle OBD-II mode 03 request (read DTCs)
    String handleMode03() {
        if (currentState.dtc_count == 0) {
            return "NO DATA\r\r>";
        }

        String response = "";
        char buf[8];

        // First byte: number of DTCs
        sprintf(buf, "%02X", currentState.dtc_count);
        response += buf;

        // Return all DTCs (2 bytes each)
        for (uint8_t i = 0; i < currentState.dtc_count; i++) {
            uint16_t dtc = currentState.dtcs[i];
            if (elm->getSpaces()) response += " ";
            sprintf(buf, "%02X", (dtc >> 8) & 0xFF);
            response += buf;
            if (elm->getSpaces()) response += " ";
            sprintf(buf, "%02X", dtc & 0xFF);
            response += buf;
        }

        response += "\r\r>";
        return response;
    }

    // Handle OBD-II mode 04 request (clear DTCs)
    String handleMode04() {
        clearDTCs();
        return "44\r\r>";  // Mode 04 response
    }

    // Handle OBD-II mode 07 request (pending DTCs)
    String handleMode07() {
        // For simplicity, we'll report no pending DTCs
        // (Real systems would track pending vs confirmed DTCs separately)
        return "NO DATA\r\r>";
    }

    // Handle OBD-II mode 09 request (vehicle information)
    String handleMode09(uint8_t pid) {
        char buf[8];
        String response = "";

        switch (pid) {
            case 0x00: {  // Supported PIDs [01-20]
                // We support: 0x02 (VIN), 0x0A (ECU name)
                if (elm->getHeaders()) {
                    response = "7E8";
                    if (elm->getSpaces()) response += " ";
                    sprintf(buf, "06");
                    response += buf;
                }
                if (elm->getSpaces()) response += " ";
                sprintf(buf, "49");
                response += buf;
                if (elm->getSpaces()) response += " ";
                sprintf(buf, "00");
                response += buf;
                if (elm->getSpaces()) response += " ";
                sprintf(buf, "54");  // 0x02 and 0x0A supported
                response += buf;
                if (elm->getSpaces()) response += " ";
                sprintf(buf, "00");
                response += buf;
                if (elm->getSpaces()) response += " ";
                sprintf(buf, "00");
                response += buf;
                if (elm->getSpaces()) response += " ";
                sprintf(buf, "00");
                response += buf;
                response += "\r\r>";
                return response;
            }

            case 0x02: {  // VIN (17 characters, multi-line response)
                // VIN is sent as multi-line CAN messages
                // Line 1: 49 02 01 XX XX XX XX (message count + first 5 chars)
                // Line 2: 49 02 02 XX XX XX XX (next 7 chars)
                // Line 3: 49 02 03 XX XX XX XX (last 5 chars)

                const char* vin = config->getVIN();
                int vinLen = strlen(vin);
                if (vinLen != 17) {
                    return "NO DATA\r\r>";
                }

                // Message 1: byte count (01) + first 5 VIN chars
                if (elm->getHeaders()) {
                    response = "7E8";
                    if (elm->getSpaces()) response += " ";
                }
                sprintf(buf, "%s49%s02%s01",
                        elm->getSpaces() ? "06 " : "06",
                        elm->getSpaces() ? " " : "",
                        elm->getSpaces() ? " " : "");
                response += buf;
                for (int i = 0; i < 5 && i < vinLen; i++) {
                    if (elm->getSpaces()) response += " ";
                    sprintf(buf, "%02X", (uint8_t)vin[i]);
                    response += buf;
                }
                response += "\r";

                // Message 2: sequence 02 + next 7 VIN chars
                if (elm->getHeaders()) {
                    response += "7E8";
                    if (elm->getSpaces()) response += " ";
                }
                sprintf(buf, "%s49%s02%s02",
                        elm->getSpaces() ? "08 " : "08",
                        elm->getSpaces() ? " " : "",
                        elm->getSpaces() ? " " : "");
                response += buf;
                for (int i = 5; i < 12 && i < vinLen; i++) {
                    if (elm->getSpaces()) response += " ";
                    sprintf(buf, "%02X", (uint8_t)vin[i]);
                    response += buf;
                }
                response += "\r";

                // Message 3: sequence 03 + last 5 VIN chars
                if (elm->getHeaders()) {
                    response += "7E8";
                    if (elm->getSpaces()) response += " ";
                }
                sprintf(buf, "%s49%s02%s03",
                        elm->getSpaces() ? "06 " : "06",
                        elm->getSpaces() ? " " : "",
                        elm->getSpaces() ? " " : "");
                response += buf;
                for (int i = 12; i < 17 && i < vinLen; i++) {
                    if (elm->getSpaces()) response += " ";
                    sprintf(buf, "%02X", (uint8_t)vin[i]);
                    response += buf;
                }
                response += "\r\r>";
                return response;
            }

            case 0x0A: {  // ECU Name
                const char* ecuName = ELM_DEVICE_ID;
                int nameLen = strlen(ecuName);
                if (nameLen > 20) nameLen = 20;  // Limit to 20 chars

                if (elm->getHeaders()) {
                    response = "7E8";
                    if (elm->getSpaces()) response += " ";
                }

                // Length byte
                uint8_t length = 2 + nameLen;  // mode + PID + name bytes
                sprintf(buf, "%02X", length);
                response += buf;
                if (elm->getSpaces()) response += " ";

                // Mode + PID
                sprintf(buf, "49%s0A", elm->getSpaces() ? " " : "");
                response += buf;

                // ECU name as ASCII bytes
                for (int i = 0; i < nameLen; i++) {
                    if (elm->getSpaces()) response += " ";
                    sprintf(buf, "%02X", (uint8_t)ecuName[i]);
                    response += buf;
                }
                response += "\r\r>";
                return response;
            }

            default:
                return "NO DATA\r\r>";
        }
    }

    String handleRequest(String request) {
        request.trim();
        request.toUpperCase();
        request.replace(" ", "");

        // Must be at least 2 characters (mode)
        if (request.length() < 2) {
            return "?\r\r>";
        }

        // Parse mode
        uint8_t mode = strtol(request.substring(0, 2).c_str(), NULL, 16);

        // Handle different modes
        switch (mode) {
            case 0x01:  // Current data
                // Parse PID (if present)
                if (request.length() >= 4) {
                    uint8_t pid = strtol(request.substring(2, 4).c_str(), NULL, 16);
                    return handleMode01(pid);
                }
                return "?\r\r>";

            case 0x03:  // Show stored DTCs
                return handleMode03();

            case 0x04:  // Clear DTCs and MIL
                return handleMode04();

            case 0x07:  // Show pending DTCs
                return handleMode07();

            case 0x09:  // Vehicle information
                // Parse PID (if present)
                if (request.length() >= 4) {
                    uint8_t pid = strtol(request.substring(2, 4).c_str(), NULL, 16);
                    return handleMode09(pid);
                }
                return "?\r\r>";

            default:
                return "NO DATA\r\r>";
        }
    }
};

#endif // PID_HANDLER_H
