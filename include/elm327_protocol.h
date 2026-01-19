#ifndef ELM327_PROTOCOL_H
#define ELM327_PROTOCOL_H

#include <Arduino.h>
#include "config.h"

class ELM327Protocol {
private:
    bool echo;
    bool headers;
    bool spaces;
    bool linefeed;
    uint8_t protocol;
    uint16_t timeout;

    // Helper to format hex bytes
    void formatHexByte(char* buf, uint8_t value, bool addSpace) {
        static const char hex[] = "0123456789ABCDEF";
        *buf++ = hex[value >> 4];
        *buf++ = hex[value & 0x0F];
        if (addSpace && spaces) *buf++ = ' ';
        *buf = '\0';
    }

public:
    ELM327Protocol() {
        reset();
    }

    void reset() {
        echo = true;  // Real Vgate iCar2 defaults to echo ON
        headers = false;
        spaces = true;
        linefeed = true;
        protocol = 0;  // Auto
        timeout = 200;
    }

    bool isEchoEnabled() const {
        return echo;
    }

    // Parse AT command and return response
    String handleCommand(String cmd) {
        cmd.trim();
        cmd.toUpperCase();

        // Remove spaces from command
        cmd.replace(" ", "");

        String response = "";

        // Echo command if enabled
        if (echo) {
            response = cmd + "\r";
        }

        // AT Command Processing
        if (cmd == "ATZ" || cmd == "AT Z") {
            // Reset
            reset();
            delay(100);
            return response + ELM_DEVICE_DESC "\r\r>";
        }
        else if (cmd == "ATI" || cmd == "AT I") {
            // Device info - returns device identifier
            return response + ELM_DEVICE_ID "\r\r>";
        }
        else if (cmd == "AT@1") {
            // Device description - returns version string
            return response + ELM_DEVICE_DESC "\r\r>";
        }
        else if (cmd == "AT@2") {
            // Not supported by real Vgate iCar2 - return error
            return response + "?\r\r>";
        }
        else if (cmd == "ATRV") {
            // Read voltage
            return response + ELM_VOLTAGE "\r\r>";
        }
        else if (cmd.startsWith("ATE")) {
            // Echo on/off
            echo = (cmd.charAt(3) == '1');
            return response + "OK\r\r>";
        }
        else if (cmd.startsWith("ATH")) {
            // Headers on/off
            headers = (cmd.charAt(3) == '1');
            return response + "OK\r\r>";
        }
        else if (cmd.startsWith("ATS")) {
            // Spaces on/off
            spaces = (cmd.charAt(3) == '1');
            return response + "OK\r\r>";
        }
        else if (cmd.startsWith("ATL")) {
            // Linefeeds on/off
            linefeed = (cmd.charAt(3) == '1');
            return response + "OK\r\r>";
        }
        else if (cmd.startsWith("ATSP")) {
            // Set protocol
            if (cmd.length() > 4) {
                protocol = cmd.substring(4).toInt();
            }
            return response + "OK\r\r>";
        }
        else if (cmd.startsWith("ATTP")) {
            // Try protocol
            if (cmd.length() > 4) {
                protocol = cmd.substring(4).toInt();
            }
            return response + "OK\r\r>";
        }
        else if (cmd.startsWith("ATST")) {
            // Set timeout
            if (cmd.length() > 4) {
                timeout = cmd.substring(4).toInt() * 4; // Input is in increments of 4ms
            }
            return response + "OK\r\r>";
        }
        else if (cmd == "ATDPN" || cmd == "ATDP") {
            // Describe current protocol
            // Don't include "AUTO, " prefix - that suggests protocol not confirmed
            // We always report as successfully detected CAN protocol
            if (cmd == "ATDPN") {
                return response + "6\r\r>";  // Protocol number for ISO 15765-4 CAN 11/500
            } else {
                return response + "ISO 15765-4 (CAN 11/500)\r\r>";
            }
        }
        else if (cmd == "ATAT0" || cmd == "ATAT1" || cmd == "ATAT2") {
            // Adaptive timing
            return response + "OK\r\r>";
        }
        else if (cmd.startsWith("ATM")) {
            // Memory on/off
            return response + "OK\r\r>";
        }
        else if (cmd.startsWith("ATCAF")) {
            // CAN auto formatting
            return response + "OK\r\r>";
        }
        else if (cmd == "ATD") {
            // Set defaults (exact match only)
            reset();
            return response + "OK\r\r>";
        }
        else if (cmd.startsWith("ATWS")) {
            // Warm start
            return response + "ELM327 v1.5\r\r>";
        }
        else if (cmd.startsWith("ATSW")) {
            // Set wakeup message (not commonly used, just acknowledge)
            return response + "OK\r\r>";
        }
        else if (cmd == "AT" || cmd.length() == 0) {
            // Empty AT command
            return response + "OK\r\r>";
        }
        else {
            // Unknown command
            return response + "?\r\r>";
        }
    }

    // Format OBD response with proper spacing and headers
    String formatOBDResponse(uint8_t mode, uint8_t pid, uint8_t* data, uint8_t dataLen) {
        String response = "";
        char buf[4];

        if (headers) {
            // Add CAN header (7E8 is typical ECU response)
            response = "7E8";
            if (spaces) response += " ";

            // Add length byte (mode + PID + data bytes)
            uint8_t length = 2 + dataLen;  // 1 byte mode + 1 byte PID + data
            formatHexByte(buf, length, spaces);
            response += buf;
        }

        // Add response mode (request mode + 0x40)
        formatHexByte(buf, mode + 0x40, spaces);
        response += buf;

        // Add PID
        formatHexByte(buf, pid, spaces);
        response += buf;

        // Add data bytes
        for (uint8_t i = 0; i < dataLen; i++) {
            formatHexByte(buf, data[i], (spaces && i < dataLen - 1));
            response += buf;
        }

        response += "\r\r>";
        return response;
    }

    bool getSpaces() { return spaces; }
    bool getHeaders() { return headers; }
};

#endif // ELM327_PROTOCOL_H
