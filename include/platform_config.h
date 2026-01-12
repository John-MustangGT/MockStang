#ifndef PLATFORM_CONFIG_H
#define PLATFORM_CONFIG_H

// Platform detection and configuration
// Build flags are set in platformio.ini for each environment

#ifdef ESP01_BUILD
    // ============================================
    // ESP-01/ESP-01S Configuration (WiFi Only)
    // ============================================
    #define PLATFORM_NAME "ESP-01S"
    #define PLATFORM_SHORT "ESP01"

    // Feature availability
    #define ENABLE_BLE false
    #define ENABLE_DISPLAY false
    #define HAS_PSRAM false

    // Resource limits (constrained)
    #define MAX_CONNECTIONS 2
    #define WEB_BUFFER_SIZE 256
    #define MAX_WS_CLIENTS 2

    // Memory optimization
    #define COMPACT_WEB_INTERFACE true
    #define MINIMAL_LOGGING true

#elif defined(ESP32_BUILD)
    // ============================================
    // ESP32-S3 Feather Configuration (Full Featured)
    // ============================================
    #define PLATFORM_NAME "ESP32-S3 Feather"
    #define PLATFORM_SHORT "ESP32S3"

    // Feature availability
    #define ENABLE_BLE true
    #define ENABLE_DISPLAY true
    #define HAS_PSRAM true

    // Resource limits (generous)
    #define MAX_CONNECTIONS 4
    #define WEB_BUFFER_SIZE 512
    #define MAX_WS_CLIENTS 4

    // Full features
    #define COMPACT_WEB_INTERFACE false
    #define MINIMAL_LOGGING false

    // BLE Configuration
    #define BLE_DEVICE_NAME "MockStang BLE"
    #define BLE_SERVICE_UUID "0000fff0-0000-1000-8000-00805f9b34fb"
    #define BLE_CHAR_UUID_RX "0000fff1-0000-1000-8000-00805f9b34fb"
    #define BLE_CHAR_UUID_TX "0000fff2-0000-1000-8000-00805f9b34fb"

    // Display Configuration
    #define DISPLAY_WIDTH 240
    #define DISPLAY_HEIGHT 135
    #define DISPLAY_ROTATION 3

#else
    #error "No platform defined! Use -DESP01_BUILD or -DESP32_BUILD in platformio.ini"
#endif

// Common feature checks
#if ENABLE_BLE
    #define BLE_AVAILABLE 1
#else
    #define BLE_AVAILABLE 0
#endif

#if ENABLE_DISPLAY
    #define DISPLAY_AVAILABLE 1
#else
    #define DISPLAY_AVAILABLE 0
#endif

// Platform capabilities struct
struct PlatformCapabilities {
    const char* name;
    const char* shortName;
    bool wifi;
    bool bluetooth;
    bool display;
    bool psram;
    uint8_t maxConnections;
};

static const PlatformCapabilities PLATFORM_CAPS = {
    .name = PLATFORM_NAME,
    .shortName = PLATFORM_SHORT,
    .wifi = true,  // All platforms have WiFi
    .bluetooth = ENABLE_BLE,
    .display = ENABLE_DISPLAY,
    .psram = HAS_PSRAM,
    .maxConnections = MAX_CONNECTIONS
};

#endif // PLATFORM_CONFIG_H
