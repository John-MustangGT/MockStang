#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <EEPROM.h>
#include <Arduino.h>
#include <IPAddress.h>

#define CONFIG_MAGIC "MOCK"
#define CONFIG_VERSION 1
#define EEPROM_SIZE 512  // Reserve 512 bytes for config

// Persistent configuration structure
struct PersistentConfig {
    char magic[4];              // "MOCK" validation marker
    uint8_t version;            // Config version

    // Network settings
    bool useCustomSSID;         // Override MAC-based SSID
    char ssid[32];              // Custom SSID
    bool usePassword;           // Enable WiFi password
    char password[64];          // WiFi password
    uint8_t ip[4];              // IP address
    uint8_t subnet[4];          // Subnet mask
    uint8_t gateway[4];         // Gateway address

    // Vehicle settings
    char vin[18];               // Vehicle VIN (17 chars + null terminator)
    char deviceId[32];          // Custom device ID for AT@1 command

    // Default PID values (optional - for custom defaults)
    uint16_t defaultRPM;
    uint8_t defaultSpeed;
    uint8_t defaultCoolantTemp;
    uint8_t defaultIntakeTemp;
    uint8_t defaultThrottle;
    uint16_t defaultMAF;
    uint8_t defaultFuelLevel;
    uint8_t defaultBarometric;

    uint16_t crc;               // CRC16 checksum
};

class ConfigManager {
private:
    PersistentConfig config;

    // Calculate CRC16 checksum
    uint16_t calculateCRC(const PersistentConfig* cfg) {
        uint16_t crc = 0xFFFF;
        const uint8_t* data = (const uint8_t*)cfg;
        size_t len = sizeof(PersistentConfig) - sizeof(uint16_t); // Exclude CRC field itself

        for (size_t i = 0; i < len; i++) {
            crc ^= data[i];
            for (uint8_t j = 0; j < 8; j++) {
                if (crc & 1) {
                    crc = (crc >> 1) ^ 0xA001;
                } else {
                    crc >>= 1;
                }
            }
        }
        return crc;
    }

    // Validate configuration
    bool isValid(const PersistentConfig* cfg) {
        // Check magic marker
        if (memcmp(cfg->magic, CONFIG_MAGIC, 4) != 0) {
            return false;
        }

        // Check version (for now, only version 1)
        if (cfg->version != CONFIG_VERSION) {
            return false;
        }

        // Verify CRC
        uint16_t calculatedCRC = calculateCRC(cfg);
        if (cfg->crc != calculatedCRC) {
            return false;
        }

        return true;
    }

public:
    ConfigManager() {
        EEPROM.begin(EEPROM_SIZE);
        loadDefaults();
    }

    // Load factory defaults
    void loadDefaults() {
        memcpy(config.magic, CONFIG_MAGIC, 4);
        config.version = CONFIG_VERSION;

        // Network defaults (match vGate iCar Pro)
        config.useCustomSSID = false;
        strcpy(config.ssid, "");
        config.usePassword = false;
        strcpy(config.password, "");
        config.ip[0] = 192; config.ip[1] = 168; config.ip[2] = 0; config.ip[3] = 10;
        config.subnet[0] = 255; config.subnet[1] = 255; config.subnet[2] = 255; config.subnet[3] = 0;
        config.gateway[0] = 192; config.gateway[1] = 168; config.gateway[2] = 0; config.gateway[3] = 10;

        // Vehicle defaults
        strcpy(config.vin, "1ZVBP8AM5D5123456");  // Example Mustang VIN
        strcpy(config.deviceId, "MockStang ESP-01S");

        // PID defaults
        config.defaultRPM = 850;
        config.defaultSpeed = 0;
        config.defaultCoolantTemp = 90;
        config.defaultIntakeTemp = 25;
        config.defaultThrottle = 0;
        config.defaultMAF = 250;
        config.defaultFuelLevel = 75;
        config.defaultBarometric = 101;

        // Calculate and store CRC
        config.crc = calculateCRC(&config);
    }

    // Load configuration from EEPROM
    bool load() {
        PersistentConfig temp;
        EEPROM.get(0, temp);

        if (isValid(&temp)) {
            memcpy(&config, &temp, sizeof(PersistentConfig));
            Serial.println("Configuration loaded from EEPROM");
            return true;
        } else {
            Serial.println("Invalid or missing configuration, using defaults");
            loadDefaults();
            return false;
        }
    }

    // Save configuration to EEPROM
    void save() {
        config.crc = calculateCRC(&config);
        EEPROM.put(0, config);
        EEPROM.commit();
        Serial.println("Configuration saved to EEPROM");
    }

    // Factory reset
    void reset() {
        loadDefaults();
        save();
        Serial.println("Configuration reset to factory defaults");
    }

    // Getters
    bool getUseCustomSSID() { return config.useCustomSSID; }
    const char* getSSID() { return config.ssid; }
    bool getUsePassword() { return config.usePassword; }
    const char* getPassword() { return config.password; }
    IPAddress getIP() { return IPAddress(config.ip[0], config.ip[1], config.ip[2], config.ip[3]); }
    IPAddress getSubnet() { return IPAddress(config.subnet[0], config.subnet[1], config.subnet[2], config.subnet[3]); }
    IPAddress getGateway() { return IPAddress(config.gateway[0], config.gateway[1], config.gateway[2], config.gateway[3]); }
    const char* getVIN() { return config.vin; }
    const char* getDeviceId() { return config.deviceId; }

    uint16_t getDefaultRPM() { return config.defaultRPM; }
    uint8_t getDefaultSpeed() { return config.defaultSpeed; }
    uint8_t getDefaultCoolantTemp() { return config.defaultCoolantTemp; }
    uint8_t getDefaultIntakeTemp() { return config.defaultIntakeTemp; }
    uint8_t getDefaultThrottle() { return config.defaultThrottle; }
    uint16_t getDefaultMAF() { return config.defaultMAF; }
    uint8_t getDefaultFuelLevel() { return config.defaultFuelLevel; }
    uint8_t getDefaultBarometric() { return config.defaultBarometric; }

    // Setters
    void setUseCustomSSID(bool use) { config.useCustomSSID = use; }
    void setSSID(const char* ssid) { strncpy(config.ssid, ssid, 31); config.ssid[31] = 0; }
    void setUsePassword(bool use) { config.usePassword = use; }
    void setPassword(const char* password) { strncpy(config.password, password, 63); config.password[63] = 0; }
    void setIP(const IPAddress& ip) {
        config.ip[0] = ip[0]; config.ip[1] = ip[1];
        config.ip[2] = ip[2]; config.ip[3] = ip[3];
    }
    void setSubnet(const IPAddress& subnet) {
        config.subnet[0] = subnet[0]; config.subnet[1] = subnet[1];
        config.subnet[2] = subnet[2]; config.subnet[3] = subnet[3];
    }
    void setGateway(const IPAddress& gateway) {
        config.gateway[0] = gateway[0]; config.gateway[1] = gateway[1];
        config.gateway[2] = gateway[2]; config.gateway[3] = gateway[3];
    }
    void setVIN(const char* vin) { strncpy(config.vin, vin, 17); config.vin[17] = 0; }
    void setDeviceId(const char* id) { strncpy(config.deviceId, id, 31); config.deviceId[31] = 0; }

    void setDefaultRPM(uint16_t rpm) { config.defaultRPM = rpm; }
    void setDefaultSpeed(uint8_t speed) { config.defaultSpeed = speed; }
    void setDefaultCoolantTemp(uint8_t temp) { config.defaultCoolantTemp = temp; }
    void setDefaultIntakeTemp(uint8_t temp) { config.defaultIntakeTemp = temp; }
    void setDefaultThrottle(uint8_t throttle) { config.defaultThrottle = throttle; }
    void setDefaultMAF(uint16_t maf) { config.defaultMAF = maf; }
    void setDefaultFuelLevel(uint8_t level) { config.defaultFuelLevel = level; }
    void setDefaultBarometric(uint8_t baro) { config.defaultBarometric = baro; }

    // Export config as JSON string
    String toJSON() {
        String json = "{";
        json += "\"useCustomSSID\":" + String(config.useCustomSSID ? "true" : "false") + ",";
        json += "\"ssid\":\"" + String(config.ssid) + "\",";
        json += "\"usePassword\":" + String(config.usePassword ? "true" : "false") + ",";
        json += "\"password\":\"" + String(config.password) + "\",";
        json += "\"ip\":\"" + String(config.ip[0]) + "." + String(config.ip[1]) + "."
                + String(config.ip[2]) + "." + String(config.ip[3]) + "\",";
        json += "\"subnet\":\"" + String(config.subnet[0]) + "." + String(config.subnet[1]) + "."
                + String(config.subnet[2]) + "." + String(config.subnet[3]) + "\",";
        json += "\"gateway\":\"" + String(config.gateway[0]) + "." + String(config.gateway[1]) + "."
                + String(config.gateway[2]) + "." + String(config.gateway[3]) + "\",";
        json += "\"vin\":\"" + String(config.vin) + "\",";
        json += "\"deviceId\":\"" + String(config.deviceId) + "\",";
        json += "\"defaultRPM\":" + String(config.defaultRPM) + ",";
        json += "\"defaultSpeed\":" + String(config.defaultSpeed) + ",";
        json += "\"defaultCoolantTemp\":" + String(config.defaultCoolantTemp) + ",";
        json += "\"defaultIntakeTemp\":" + String(config.defaultIntakeTemp) + ",";
        json += "\"defaultThrottle\":" + String(config.defaultThrottle) + ",";
        json += "\"defaultMAF\":" + String(config.defaultMAF) + ",";
        json += "\"defaultFuelLevel\":" + String(config.defaultFuelLevel) + ",";
        json += "\"defaultBarometric\":" + String(config.defaultBarometric);
        json += "}";
        return json;
    }
};

#endif // CONFIG_MANAGER_H
