#ifndef CONFIG_H
#define CONFIG_H

// WiFi AP Configuration
// SSID will be generated as "iCAR_PRO_XXXX" where XXXX = last 2 octets of MAC
#define WIFI_SSID_PREFIX "iCAR_PRO_"
#define WIFI_PASSWORD ""  // vGate iCar Pro default: no password (open network)
#define WIFI_CHANNEL 6
#define MAX_CONNECTIONS 1

// Network Configuration (vGate iCar Pro defaults)
#define AP_IP_ADDRESS IPAddress(192, 168, 0, 10)
#define AP_GATEWAY IPAddress(192, 168, 0, 10)
#define AP_SUBNET IPAddress(255, 255, 255, 0)

// Server Ports
#define ELM327_PORT 35000
#define WEB_SERVER_PORT 80

// ELM327 Protocol Settings
#define ELM_DEVICE_DESC "ELM327 v1.5"
#define ELM_DEVICE_ID "MockStang ESP-01S"
#define ELM_VOLTAGE "12.8V"  // Simulated car voltage

// Default PID Values (adjustable via web interface)
struct CarState {
    uint16_t rpm;           // 0x0C - Engine RPM
    uint8_t speed;          // 0x0D - Vehicle Speed (km/h)
    uint8_t coolant_temp;   // 0x05 - Coolant Temperature (°C + 40)
    uint8_t intake_temp;    // 0x0F - Intake Air Temperature (°C + 40)
    uint8_t throttle;       // 0x11 - Throttle Position (%)
    uint16_t maf;           // 0x10 - MAF Air Flow (grams/sec * 100)
    uint16_t runtime;       // 0x1F - Run time since engine start (seconds)
    uint16_t mil_distance;  // 0x21 - Distance traveled with MIL on (km)
    uint8_t fuel_level;     // 0x2F - Fuel Tank Level (%)
    uint8_t barometric;     // 0x33 - Barometric Pressure (kPa)
};

// Default car state (typical idle conditions)
const CarState DEFAULT_CAR_STATE = {
    .rpm = 850,              // Idle RPM
    .speed = 0,              // Stopped
    .coolant_temp = 90,      // 90°C normal operating temp
    .intake_temp = 25,       // 25°C ambient
    .throttle = 0,           // Closed
    .maf = 250,              // 2.5 g/s at idle
    .runtime = 300,          // 5 minutes
    .mil_distance = 0,       // No codes
    .fuel_level = 75,        // 75% full
    .barometric = 101        // Sea level pressure
};

// Memory optimization
#define MAX_COMMAND_LENGTH 64
#define MAX_RESPONSE_LENGTH 128
#define WEBSOCKET_BUFFER_SIZE 256

#endif // CONFIG_H
