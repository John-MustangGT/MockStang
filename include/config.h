#ifndef CONFIG_H
#define CONFIG_H

#include "platform_config.h"

// WiFi AP Configuration
// SSID will be generated as "iCAR_PRO_XXXX" where XXXX = last 2 octets of MAC
#define WIFI_SSID_PREFIX "iCAR_PRO_"
#define WIFI_PASSWORD ""  // vGate iCar Pro default: no password (open network)
#define WIFI_CHANNEL 6
// MAX_CONNECTIONS is defined in platform_config.h (2 for ESP-01, 4 for ESP32)

// Network Configuration (vGate iCar Pro defaults)
#define AP_IP_ADDRESS IPAddress(192, 168, 0, 10)
#define AP_GATEWAY IPAddress(192, 168, 0, 10)
#define AP_SUBNET IPAddress(255, 255, 255, 0)

// Server Ports
#define ELM327_PORT 35000
#define WEB_SERVER_PORT 80

// ELM327 Protocol Settings
#define ELM_DEVICE_DESC "ELM327 v2.1"  // Match real Vgate iCar2 version
// Use Vgate-compatible device ID for OBD app recognition
#define ELM_DEVICE_ID "OBDII to RS232 Interpreter"
#define ELM_VOLTAGE "11.8V"  // Typical car accessory voltage (match real adapter)

// Serial Debugging
#define ENABLE_SERIAL_LOGGING true  // Enable/disable CMD/RESP logging to serial

// Maximum number of stored DTCs
#define MAX_DTCS 8

// Driving Simulator Modes
enum DriveMode {
    DRIVE_OFF = 0,       // Manual control only
    DRIVE_GENTLE = 1,    // Gentle acceleration, warming up (0-50 km/h in 5s)
    DRIVE_NORMAL = 2,    // Normal driving (0-80 km/h in 7s)
    DRIVE_SPORT = 3,     // Sporty driving (0-120 km/h in 8s)
    DRIVE_DRAG = 4       // Drag race (0-180 km/h in 12s, full throttle)
};

// Default PID Values (adjustable via web interface)
struct CarState {
    // Core PIDs (Mode 01)
    uint16_t rpm;           // 0x0C - Engine RPM
    uint8_t speed;          // 0x0D - Vehicle Speed (km/h)
    uint8_t coolant_temp;   // 0x05 - Coolant Temperature (°C)
    uint8_t intake_temp;    // 0x0F - Intake Air Temperature (°C)
    uint8_t throttle;       // 0x11 - Throttle Position (%)
    uint16_t maf;           // 0x10 - MAF Air Flow (grams/sec * 100)
    uint16_t runtime;       // 0x1F - Run time since engine start (seconds)
    uint16_t mil_distance;  // 0x21 - Distance traveled with MIL on (km)
    uint8_t fuel_level;     // 0x2F - Fuel Tank Level (%)
    uint8_t barometric;     // 0x33 - Barometric Pressure (kPa)

    // Additional common PIDs
    int8_t short_fuel_trim; // 0x06 - Short term fuel trim Bank 1 (-100% to +99%)
    int8_t long_fuel_trim;  // 0x07 - Long term fuel trim Bank 1 (-100% to +99%)
    uint8_t map;            // 0x0B - Intake manifold absolute pressure (kPa)
    int8_t timing_advance;  // 0x0E - Timing advance (-64° to +63°)
    uint8_t o2_voltage;     // 0x14 - O2 sensor voltage (0-1.275V, *200)
    uint16_t fuel_pressure; // 0x23 - Fuel rail pressure (kPa)
    uint8_t egr;            // 0x2C - Commanded EGR (%)
    uint16_t distance_mil_clear; // 0x31 - Distance since codes cleared (km)
    uint16_t battery_voltage; // 0x42 - Control module voltage (millivolts)
    uint8_t ambient_temp;   // 0x46 - Ambient air temperature (°C)
    uint8_t oil_temp;       // 0x5C - Engine oil temperature (°C)

    // MIL and DTC management
    bool mil_on;            // MIL (Check Engine Light) status
    uint8_t dtc_count;      // Number of stored DTCs
    uint16_t dtcs[MAX_DTCS]; // Stored DTCs (P0xxx format as 16-bit values)
};

// Default car state (typical idle conditions)
const CarState DEFAULT_CAR_STATE = {
    // Core PIDs
    .rpm = 850,              // Idle RPM
    .speed = 0,              // Stopped
    .coolant_temp = 90,      // 90°C normal operating temp
    .intake_temp = 25,       // 25°C ambient
    .throttle = 0,           // Closed
    .maf = 250,              // 2.5 g/s at idle
    .runtime = 300,          // 5 minutes
    .mil_distance = 0,       // No codes
    .fuel_level = 75,        // 75% full
    .barometric = 101,       // Sea level pressure
    // Additional PIDs
    .short_fuel_trim = 0,    // 0% trim (perfect mixture)
    .long_fuel_trim = 2,     // +2% trim (slight lean compensation)
    .map = 35,               // 35 kPa at idle (vacuum)
    .timing_advance = 15,    // 15° advance at idle
    .o2_voltage = 90,        // 0.45V (~14.7:1 AFR)
    .fuel_pressure = 380,    // 380 kPa (55 psi) typical fuel pressure
    .egr = 0,                // 0% EGR at idle
    .distance_mil_clear = 1500, // 1500 km since last clear
    .battery_voltage = 14200, // 14.2V (charging)
    .ambient_temp = 20,      // 20°C ambient
    .oil_temp = 95,          // 95°C normal oil temp
    // MIL and DTC management
    .mil_on = false,         // MIL off by default
    .dtc_count = 0,          // No DTCs
    .dtcs = {0}              // Empty DTC array
};

// Memory optimization (platform-specific in platform_config.h)
#define MAX_COMMAND_LENGTH 64
#define MAX_RESPONSE_LENGTH 128
// WEBSOCKET_BUFFER_SIZE is defined in platform_config.h (256 for ESP-01, 512 for ESP32)

#endif // CONFIG_H
