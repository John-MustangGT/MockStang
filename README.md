# MockStang - WiFi OBD-II Emulator

A WiFi-based OBD-II adapter emulator for ESP-01S that mimics the vGate iCar Pro WiFi adapter. Perfect for developing and testing OBD-II applications like OpenPonyLogger without needing a real vehicle.

## Features

- **ELM327 v1.5 Protocol Emulation** - Full AT command support
- **WiFi Access Point Mode** - Emulates vGate iCar 3 WiFi adapter
- **Real-time Web Dashboard** - Monitor and control mock data via browser
- **WebSocket Updates** - Live command/response monitoring
- **Adjustable PIDs** - Modify car parameters on-the-fly with sliders
- **Optimized for ESP-01S** - Runs on 1MB flash with ~50KB RAM

## Hardware Requirements

- **ESP-01S** (1MB flash recommended)
- **USB to Serial adapter** (for programming)
- **3.3V power supply**

## Supported OBD-II PIDs

### Core PIDs
- `0x0C` - Engine RPM
- `0x0D` - Vehicle Speed (km/h)
- `0x05` - Engine Coolant Temperature (°C)
- `0x0F` - Intake Air Temperature (°C)
- `0x11` - Throttle Position (%)
- `0x10` - MAF Air Flow Rate (g/s)
- `0x1F` - Run Time Since Engine Start (s)
- `0x2F` - Fuel Tank Level (%)
- `0x33` - Barometric Pressure (kPa)
- `0x21` - Distance Traveled with MIL On (km)

### Mandatory PIDs
- `0x00` - PIDs supported [01-20]
- `0x01` - Monitor status since DTCs cleared
- `0x03` - Fuel system status
- `0x04` - Calculated engine load value
- `0x20` - PIDs supported [21-40]
- `0x40` - PIDs supported [41-60]

## Installation

### Using PlatformIO (Recommended)

1. Install [PlatformIO](https://platformio.org/install)
2. Clone this repository:
   ```bash
   git clone <repo-url>
   cd MockStang
   ```
3. Install dependencies:
   ```bash
   pio pkg install
   ```
4. Build the project:
   ```bash
   pio run
   ```
5. Upload to ESP-01S:
   ```bash
   pio run --target upload
   ```

### Using Arduino IDE

1. Install the following libraries via Library Manager:
   - ESPAsyncWebServer
   - ESPAsyncTCP
   - ESP8266WiFi (included with ESP8266 board support)

2. Copy all files from `include/` to your sketch folder
3. Open `src/mockstang.ino` in Arduino IDE
4. Select **Board**: Generic ESP8266 Module
5. Set **Flash Size**: 1MB (FS:256KB OTA:~374KB)
6. Click Upload

## Configuration

Edit `include/config.h` to customize:

```cpp
#define WIFI_SSID_PREFIX "iCAR_PRO_"    // SSID prefix (MAC-based suffix added automatically)
#define WIFI_PASSWORD ""                // WiFi password (empty = open network, default for vGate iCar Pro)
#define AP_IP_ADDRESS IPAddress(192, 168, 0, 10)  // vGate iCar Pro default IP
#define ELM327_PORT 35000               // OBD-II port
#define WEB_SERVER_PORT 80              // Web dashboard port
```

Default car state values can also be adjusted in `config.h`.

## Usage

### 1. Power Up the ESP-01S

After uploading, the ESP-01S will:
- Create a WiFi Access Point named **"iCAR_PRO_XXXX"** (where XXXX = last 2 MAC octets)
- Use IP address **192.168.0.10**
- Start ELM327 server on port **35000**
- Start web server on port **80**

### 2. Connect to WiFi

On your phone/tablet/computer:
- Connect to WiFi network: **iCAR_PRO_XXXX** (check serial monitor for exact name)
- **No password required** (open network, default vGate iCar Pro behavior)

### 3. Access Web Dashboard

Open a browser and navigate to:
```
http://192.168.0.10
```

The dashboard allows you to:
- Monitor real-time OBD-II commands from your app
- Adjust car parameters (RPM, speed, temperature, etc.)
- Reset runtime counter
- View command/response log

### 4. Connect Your OBD-II App

Configure your OBD-II application:
- **Type**: WiFi adapter
- **IP Address**: 192.168.0.10
- **Port**: 35000
- **Protocol**: Auto or ISO 15765-4 (CAN)

## Supported AT Commands

MockStang supports the following ELM327 AT commands:

| Command | Description |
|---------|-------------|
| `ATZ` | Reset device |
| `ATI` | Print version ID |
| `AT@1` | Display device description |
| `ATRV` | Read input voltage |
| `ATE0/1` | Echo off/on |
| `ATH0/1` | Headers off/on |
| `ATS0/1` | Spaces off/on |
| `ATL0/1` | Linefeeds off/on |
| `ATSP<n>` | Set protocol |
| `ATTP<n>` | Try protocol |
| `ATST<n>` | Set timeout |
| `ATDP` | Describe protocol |
| `ATDPN` | Describe protocol by number |
| `ATAT0/1/2` | Adaptive timing |
| `ATD` | Set defaults |
| `ATWS` | Warm start |

## Serial Monitor Output

Connect via serial at **115200 baud** to see:
- Startup information
- WiFi AP details
- Client connections
- Command/response logging
- Parameter updates

Example output:
```
=================================
MockStang - WiFi OBD-II Emulator
=================================

Configuring Access Point: iCAR_PRO_A1B2
AP IP address: 192.168.0.10
AP Password: (none - open network)
AP MAC: DE:AD:BE:EF:A1:B2
ELM327 server listening on port 35000
Web server started on port 80

=================================
System Ready!
Connect to WiFi: iCAR_PRO_A1B2
OBD-II Port: 35000
Web Dashboard: http://192.168.0.10
=================================
```

## Development & Testing

### Testing with OpenPonyLogger

1. Power up MockStang
2. Check serial monitor for WiFi name (e.g., "iCAR_PRO_A1B2")
3. Connect phone to "iCAR_PRO_XXXX" WiFi (no password required)
4. Open OpenPonyLogger
5. Configure connection:
   - Type: WiFi
   - IP: 192.168.0.10
   - Port: 35000
6. Open web dashboard on laptop/tablet at http://192.168.0.10
7. Start logging in OpenPonyLogger
8. Adjust parameters in real-time via dashboard

### Simulating Driving Scenarios

Use the web dashboard sliders to simulate:
- **Idle**: RPM=850, Speed=0, Throttle=0%
- **Cruising**: RPM=2000, Speed=60, Throttle=20%
- **Acceleration**: Increase RPM (1000→5000), increase throttle (0→80%)
- **Cold Start**: Set coolant temp to 20°C, watch it rise
- **Highway**: RPM=2500, Speed=100, Throttle=30%

## Project Structure

```
MockStang/
├── include/
│   ├── config.h              # Configuration and defaults
│   ├── elm327_protocol.h     # ELM327 AT command parser
│   ├── pid_handler.h         # OBD-II PID response engine
│   ├── web_server.h          # HTTP & WebSocket server
│   └── web_interface.h       # Embedded HTML/JS dashboard
├── src/
│   └── mockstang.ino         # Main application
├── platformio.ini            # PlatformIO configuration
└── README.md                 # This file
```

## Memory Optimization

The code is optimized for ESP-01S's limited resources:
- Compiled with `-Os` (optimize for size)
- PROGMEM storage for HTML/strings
- Minimal buffer sizes
- Function/data section garbage collection
- Low-memory LWIP configuration

## Troubleshooting

### Can't connect to WiFi
- Check serial monitor for exact SSID (format: iCAR_PRO_XXXX)
- Network is open (no password required)
- Make sure ESP-01S is powered properly (3.3V, adequate current)
- Try rebooting the ESP-01S

### OBD app can't connect
- Verify you're connected to iCAR_PRO_XXXX WiFi
- Verify IP is set to 192.168.0.10 (not 192.168.4.1)
- Check port number (should be 35000)
- Try pinging 192.168.0.10
- Check serial monitor for connection attempts

### Web dashboard not loading
- Verify WiFi connection
- Try http://192.168.0.10 (not https, not 192.168.4.1)
- Clear browser cache
- Check serial monitor for web server status

### Upload fails
- Put ESP-01S in programming mode (GPIO0 to GND during power-up)
- Check serial adapter voltage (must be 3.3V, NOT 5V)
- Verify TX/RX connections are crossed
- Try slower upload speed (57600 baud)

## License

See LICENSE file for details.

## Contributing

This project was created for developing OpenPonyLogger. Feel free to submit issues or pull requests!

## Acknowledgments

- Emulates the vGate iCar Pro WiFi OBD-II adapter
- Compatible with ELM327 v1.5 protocol specification
- Built for the ESP8266 community

---

**Happy Testing!** 🏎️💨
