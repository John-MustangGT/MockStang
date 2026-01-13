# BLE Scanner for ELM327 OBD-II Adapters

A Bluetooth Low Energy (BLE) scanner and GATT service enumerator designed to discover and analyze the service/characteristic UUIDs used by ELM327 BLE OBD-II adapters.

## Purpose

Different BLE OBD-II adapters (Vgate iCar BLE, generic ELM327 BLE, etc.) use different GATT service and characteristic UUIDs. Some use custom UUIDs, others use standard Serial Port Profile. This tool helps you:

1. **Discover** BLE devices matching "iCar", "Vgate", "OBD", "ELM327" patterns
2. **Connect** to target devices
3. **Enumerate** all GATT services and characteristics
4. **Identify** which characteristics handle UART-style RX/TX for ELM327 commands
5. **Dump** complete UUID mappings for implementation in MockStang

## Hardware Requirements

- **Adafruit Feather ESP32-S3 Reverse TFT**
  - ESP32-S3 with built-in BLE
  - 1.14" 240x135 TFT display (ST7789)
  - 3 user buttons (A, B, C)
  - USB-C connector

## Features

- **Active BLE scanning** with RSSI display
- **Automatic target detection** (highlights OBD-II adapter-like devices)
- **Complete GATT enumeration** (services, characteristics, descriptors)
- **Property analysis** (READ, WRITE, NOTIFY, etc.)
- **Color TFT display** for at-a-glance status
- **Button controls** for easy operation
- **Detailed serial output** with formatted UUID dumps
- **Wireshark-ready** (can capture BLE traffic on Linux)

## Installation

### Using PlatformIO (Recommended)

1. Navigate to the tool directory:
   ```bash
   cd tools/ble-scanner
   ```

2. Install dependencies:
   ```bash
   pio pkg install
   ```

3. Build the project:
   ```bash
   pio run
   ```

4. Upload to Adafruit Feather ESP32-S3:
   ```bash
   pio run --target upload
   ```

### Using Arduino IDE

1. Install the following libraries via Library Manager:
   - **NimBLE-Arduino** by h2zero (v1.4.1+)
   - **Adafruit ST7735 and ST7789 Library**
   - **Adafruit GFX Library**
   - **Adafruit BusIO**

2. Open `src/ble-scanner.ino` in Arduino IDE

3. Select **Board**: Adafruit Feather ESP32-S3 Reverse TFT

4. Select **Port**: Your ESP32-S3 USB port

5. Click Upload

## Usage

### Basic Operation

1. **Power up** the Feather ESP32-S3
   - TFT displays "Ready! Press A to scan"

2. **Button A** - Start BLE scan
   - Scans for 10 seconds
   - Displays discovered devices on TFT
   - Green text = target device (OBD-II adapter)
   - White text = other BLE device

3. **Button C** - Navigate device list
   - Cycles through discovered devices
   - Selected device highlighted in blue

4. **Button B** - Connect to selected device
   - Connects via BLE
   - Enumerates all GATT services/characteristics
   - Dumps complete UUID mapping to serial

5. **Button A** (while connected) - Disconnect
   - Returns to device list

### Serial Monitor Output

Connect via serial at **115200 baud** to see detailed output:

```
╔════════════════════════════════════════════════════════════════╗
║                                                                ║
║          BLE Scanner for ELM327 OBD-II Adapters                ║
║                                                                ║
║  Hardware: Adafruit Feather ESP32-S3 Reverse TFT              ║
║  Purpose: Discover GATT services/characteristics              ║
║                                                                ║
╚════════════════════════════════════════════════════════════════╝

Initializing NimBLE...
✓ BLE initialized

Controls:
  Button A: Start/Stop Scan
  Button B: Connect to device
  Button C: Next device

Ready! Press Button A to start scanning...

╔════════════════════════════════════════════════════════════════╗
║                    STARTING BLE SCAN                           ║
║  Looking for: iCar, Vgate, OBD, ELM327, OBDII                  ║
╚════════════════════════════════════════════════════════════════╝

=========================================
Found Device: Vgate iCar Pro
  Address: 12:34:56:78:9a:bc
  RSSI: -56 dBm
  Target: YES
  Service UUID: 0000fff0-0000-1000-8000-00805f9b34fb
=========================================

╔════════════════════════════════════════════════════════════════╗
║                    SCAN COMPLETE                               ║
║  Found 8 devices total                                         ║
║  Found 2 target devices                                        ║
╚════════════════════════════════════════════════════════════════╝

╔════════════════════════════════════════════════════════════════╗
║ Connecting to: Vgate iCar Pro                                  ║
║ Address: 12:34:56:78:9a:bc                                     ║
╚════════════════════════════════════════════════════════════════╝

✓ Connected successfully!
MTU: 247 bytes

╔════════════════════════════════════════════════════════════════╗
║          GATT SERVICE AND CHARACTERISTIC ENUMERATION           ║
╚════════════════════════════════════════════════════════════════╝

Found 3 services

┌────────────────────────────────────────────────────────────────┐
│ SERVICE: 0000fff0-0000-1000-8000-00805f9b34fb
├────────────────────────────────────────────────────────────────┤
│ ⭐ LIKELY ELM327 SERVICE (Custom UUID)
│ Characteristics: 2
│
│  └─ CHARACTERISTIC: 0000fff1-0000-1000-8000-00805f9b34fb
│      ⭐ LIKELY TX (Write to device)
│      Properties: WRITE WRITE_NR
│
│  └─ CHARACTERISTIC: 0000fff2-0000-1000-8000-00805f9b34fb
│      ⭐ LIKELY RX (Read from device/Notify)
│      Properties: READ NOTIFY
│      Descriptors:
│        • 00002902-0000-1000-8000-00805f9b34fb (Client Characteristic Config)
│
└────────────────────────────────────────────────────────────────┘

╔════════════════════════════════════════════════════════════════╗
║                     ENUMERATION COMPLETE                       ║
╚════════════════════════════════════════════════════════════════╝
```

## Understanding the Output

### Common UUID Patterns

#### Vgate iCar BLE (Common)
- **Service**: `0000fff0-0000-1000-8000-00805f9b34fb`
- **TX (Write)**: `0000fff1-0000-1000-8000-00805f9b34fb`
- **RX (Notify)**: `0000fff2-0000-1000-8000-00805f9b34fb`

#### Nordic UART Service (NUS)
- **Service**: `6e400001-b5a3-f393-e0a9-e50e24dcca9e`
- **TX (Write)**: `6e400002-b5a3-f393-e0a9-e50e24dcca9e`
- **RX (Notify)**: `6e400003-b5a3-f393-e0a9-e50e24dcca9e`

#### Standard Serial Port Profile
- **Service**: `00001101-0000-1000-8000-00805f9b34fb`

### Characteristic Properties

- **READ** - Can read current value
- **WRITE** - Write with response (acknowledged)
- **WRITE_NR** - Write without response (faster, no ACK)
- **NOTIFY** - Server pushes updates to client
- **INDICATE** - Like NOTIFY but with acknowledgment

### Identifying RX/TX Characteristics

For ELM327 adapters, look for:

1. **TX (Client → Device)**: Usually has `WRITE` or `WRITE_NR` property
2. **RX (Device → Client)**: Usually has `NOTIFY` or `INDICATE` property

The characteristic with **NOTIFY** is almost always the one that sends ELM327 responses back to you.

## Wireshark BLE Capture (Linux)

For deeper analysis, you can capture BLE traffic:

### Prerequisites

```bash
# Install Wireshark with Bluetooth support
sudo apt-get install wireshark bluez

# Add user to wireshark group
sudo usermod -aG wireshark $USER

# Reboot or re-login
```

### Capture BLE Traffic

1. **Start Wireshark**:
   ```bash
   wireshark
   ```

2. **Select Bluetooth interface** (usually `bluetooth0` or `hci0`)

3. **Apply filter**:
   ```
   bthci_acl || btatt
   ```

4. **Run BLE scanner** on ESP32-S3

5. **Analyze** ATT protocol packets to see GATT operations

### Finding UUIDs in Wireshark

Look for these packet types:
- **Read By Group Type Request** (discovers services)
- **Read By Type Request** (discovers characteristics)
- **Handle Value Notification** (NOTIFY data from device)
- **Write Request/Command** (data sent to device)

## Troubleshooting

### No devices found

- Ensure your BLE OBD-II adapter is powered on and in pairing mode
- Some adapters only advertise when not connected to another device
- Try moving closer to the adapter (< 1 meter)
- Check serial output for scan activity

### Connection fails

- Adapter may be already connected to another device (phone/tablet)
- Some adapters require pairing/bonding first
- Try power cycling the adapter
- Check RSSI (signal strength) - should be > -80 dBm

### Display shows nothing

- Check TFT backlight (GPIO 45)
- Verify SPI connections
- Try different display rotation in code

### Serial output garbled

- Ensure baud rate is set to 115200
- Try different USB cable (some are charge-only)
- Check USB CDC is enabled in build flags

## Implementing UUIDs in MockStang

Once you've identified the UUIDs, you can implement BLE GATT server in MockStang:

```cpp
// Example UUIDs from Vgate iCar BLE
#define SERVICE_UUID        "0000fff0-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_TX   "0000fff1-0000-1000-8000-00805f9b34fb"  // Client writes here
#define CHARACTERISTIC_RX   "0000fff2-0000-1000-8000-00805f9b34fb"  // Server notifies here

// Create BLE server with these UUIDs
// TX characteristic receives ELM327 commands
// RX characteristic sends back responses via NOTIFY
```

## Hardware Pinout

### Adafruit Feather ESP32-S3 Reverse TFT

```
TFT Display:
  CS:        GPIO 7
  DC:        GPIO 39
  RST:       GPIO 40
  BACKLIGHT: GPIO 45
  MOSI:      GPIO 35 (SPI)
  SCK:       GPIO 36 (SPI)

Buttons:
  Button A:  GPIO 9  (left)
  Button B:  GPIO 6  (middle)
  Button C:  GPIO 5  (right)

USB:
  USB CDC enabled (UART over USB)
```

## Technical Details

### BLE Scan Parameters

- **Scan type**: Active (requests scan response data)
- **Scan interval**: 100ms
- **Scan window**: 99ms
- **Scan duration**: 10 seconds

### Memory Usage

- **NimBLE**: ~70KB RAM (much lighter than Arduino BLE)
- **Display buffer**: ~5KB
- **Device list**: Dynamic (std::vector)

### Power Consumption

- **Scanning**: ~80mA @ 3.3V
- **Connected**: ~60mA @ 3.3V
- **Idle**: ~40mA @ 3.3V

## Next Steps

1. **Identify UUIDs** from your specific BLE OBD-II adapter
2. **Document** the service/characteristic mappings
3. **Test** sending ELM327 commands (e.g., "ATZ", "01 0C")
4. **Implement** BLE GATT server in MockStang using discovered UUIDs
5. **Validate** with OpenPonyLogger

## Contributing

If you discover UUIDs from other BLE OBD-II adapters, please contribute them to this README!

## License

See main MockStang LICENSE file.

---

**Happy Scanning!** 🔍📡
