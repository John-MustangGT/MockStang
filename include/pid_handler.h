#ifndef PID_HANDLER_H
#define PID_HANDLER_H

#include <Arduino.h>
#include "config.h"
#include "elm327_protocol.h"

class PIDHandler {
private:
    CarState currentState;
    ELM327Protocol* elm;
    unsigned long startTime;

public:
    PIDHandler(ELM327Protocol* elmProtocol) : elm(elmProtocol) {
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
                data[1] = 0b00011110;  // 0x0C, 0x0D, 0x0F, 0x10, 0x11
                data[2] = 0b10000000;  // 0x1F
                data[3] = 0b00000001;  // 0x20 (supports next range)
                dataLen = 4;
                break;

            case 0x01:  // Monitor status since DTCs cleared
                // MIL off, 0 DTCs, tests available/complete
                data[0] = 0x00;  // MIL off, 0 DTCs
                data[1] = 0x07;  // Test available flags
                data[2] = 0x65;  // Test complete flags
                data[3] = 0x04;  // Additional flags
                dataLen = 4;
                break;

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

            case 0x0C:  // Engine RPM
                // RPM is encoded as (RPM * 4) as 16-bit big-endian
                // Formula: RPM = ((A * 256) + B) / 4
                uint16_t encodedRPM = currentState.rpm * 4;
                data[0] = encodedRPM >> 8;
                data[1] = encodedRPM & 0xFF;
                dataLen = 2;
                break;

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
                data[1] = 0b00000000;
                data[2] = 0b00001000;  // 0x2F supported
                data[3] = 0b00010001;  // 0x33 supported, 0x40 supported
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

        // We only support mode 01 (current data)
        if (mode != 0x01) {
            return "NO DATA\r\r>";
        }

        // Parse PID (if present)
        if (request.length() >= 4) {
            uint8_t pid = strtol(request.substring(2, 4).c_str(), NULL, 16);
            return handleMode01(pid);
        }

        return "?\r\r>";
    }
};

#endif // PID_HANDLER_H
